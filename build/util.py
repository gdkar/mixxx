from SCons import Script
import os
import os.path
import re
import stat

CURRENT_VCS = None


def get_current_vcs():
    if CURRENT_VCS is not None: return CURRENT_VCS
    elif on_git():              return "git"
    else:                       return "tar"
def on_git():
    cwd = os.getcwd()
    basename = " "
    while len(basename) > 0:
        try:
            os.stat(os.path.join(cwd, ".git"))
            return True
        except: pass
        cwd, basename = os.path.split(cwd)
    return False
def get_revision():
    global CURRENT_VCS
    if CURRENT_VCS is None:
        CURRENT_VCS = get_current_vcs()
    if CURRENT_VCS == "git":
        return get_git_revision()
    if CURRENT_VCS == "tar":
        return ""
    return None
def get_modified():
    global CURRENT_VCS
    if CURRENT_VCS is None:
        CURRENT_VCS = get_current_vcs()
    if CURRENT_VCS == "git":
        return get_git_modified()
    if CURRENT_VCS == "tar":
        return ""
    return None
def get_branch_name():
    global CURRENT_VCS
    if CURRENT_VCS is None:
        CURRENT_VCS = get_current_vcs()
    if CURRENT_VCS == "git":
        return get_git_branch_name()
    if CURRENT_VCS == "tar":
        return ""
    return None
def export_source(source, dest):
    global CURRENT_VCS
    if CURRENT_VCS is None:
        CURRENT_VCS = get_current_vcs()
    if CURRENT_VCS == "git":
        return export_git(source, dest)
    return None
def get_git_revision(): return len(os.popen("git log --pretty=oneline --first-parent").read().splitlines())
def get_git_modified():
    modified_matcher = re.compile( "^#.*:   (?P<filename>.*?)$")  # note output might be translated
    modified_files = []
    for line in os.popen("git status").read().splitlines():
        match = modified_matcher.match(line)
        if match:
            match = match.groupdict()
            modified_files.append(match['filename'].strip())
    return "\n".join(modified_files)
def get_git_branch_name():
    # this returns the branch name or 'HEAD' in case of detached HEAD
    branch_name = os.popen(
        "git rev-parse --abbrev-ref HEAD").readline().strip()
    if branch_name == 'HEAD':
        branch_name = '(no branch)'
    return branch_name
def export_git(source, dest):
    os.mkdir(dest)
    return os.system('git archive --format tar HEAD {} | tar x -C {}'.format (source, dest))
def get_build_dir(platformString, bitwidth):
    build_dir = '{}{}_build'.format(platformString[0:3], bitwidth)
    return build_dir
def get_mixxx_version():
    """Get Mixxx version number from defs_version.h"""
    # have to handle out-of-tree building, that's why the '#' :(
    return "1.12-beta"
def get_flags(env, argflag, default=0):
    """
    * get value passed as an argument to scons as argflag=value
    * if no value is passed to scons use stored value
    * if no value is stored, use default
    Returns the value and stores it in env[argflag]
    """
    flags = Script.ARGUMENTS.get(argflag, -1)
    if int(flags) < 0: 
        flags = env.get(argflag,default);
    env[argflag] = flags
    return flags
# Checks for pkg-config on Linux
def CheckForPKGConfig(context, version='0.0.0'):
    context.Message("Checking for pkg-config (at least version {})... ".format(version))
    ret = context.TryAction("pkg-config --atleast-pkgconfig-version={}".format(version))[0]
    context.Result(ret)
    return ret
# Uses pkg-config to check for a minimum version
def CheckForPKG(context, name, version=""):
    if version == "":
        context.Message("Checking for {}... \t".format(name))
        ret = context.TryAction("pkg-config --exists '{}'".format(name))[0]
    else:
        context.Message("Checking for {} ({} or higher)... \t".format (name, version))
        ret = context.TryAction("pkg-config --atleast-version={} '{}'".format (version, name))[0]
        context.Result(ret)
    return ret
def write_build_header(path):
    f = open(path, 'w')
    try:
        branch_name    = get_branch_name()
        modified = len(get_modified()) > 0
        # Do not emit BUILD_BRANCH on release branches.
        if branch_name and not branch_name.startswith('release'):
            f.write('#define BUILD_BRANCH "{}"\n'.format(branch_name))
        f.write('#define BUILD_REV "{}{}"\n'.format(get_revision(),'+' if modified else ''))
        f.write('#define BUILD_VERSION "{}"\n'.format(get_mixxx_version()))
    finally:
        f.close()
        os.chmod(path, stat.S_IRWXU | stat.S_IRWXG |stat.S_IRWXO)
