
MixxxKeyboardDevice = function(){
  var MixxxDebug = this.MixxxDebug   = function MixxxDebug(msg){print("[MixxxKeyboard] "+msg);}
  var MixxxKeyEvent = this.MixxxKeyEvent = function MixxxKeyEvent(data){
      this.code      = data[0] + (data[1]<<8) + (data[2]<<16);
      this.mods      = data[3] << 24;
      this.scan      = data[4] + (data[5]<<8) + (data[6]<<16) + (data[7]<<24);
      this.type      = data[8] + (data[9]<<8) + (data[10]<<16) + (data[11]<<24);
  };
  MixxxKeyEvent.prototype.keyCode = function keyCode(){
    return String.fromCharCode(this.code&0xFFFFFF);
  };
  MixxxKeyEvent.prototype.modString = function modString(){
    var ret = "";
    var i = 0x02000000;
    for(;i< 0x80000000;i*=2)
      if(i&this.mods)ret+=this.Modifiers[i]+'+';
    return ret;
  };
  MixxxKeyEvent.prototype.typeString = function typeString(){
    return this.EventTypes[this.type];
  };
  MixxxKeyEvent.prototype.Modifiers = {
    0x00000000: "None",
    0x02000000: "Shift",
    0x04000000: "Ctrl",
    0x08000000: "Alt",
    0x10000000: "Meta",
    0x20000000: "Numlock",
    0x40000000: "Modeswitch"
  };
  MixxxKeyEvent.prototype.EventTypes = {
    6         : "KeyPress",
    7         : "KeyRelease",
    51        : "Shortcut",
    117       : "ShortcutOverride",
    169       : "KeyboardLayoutChange",
  };
  var incomingData = this.incomingData = function IncomingData(data,len){
    var evt = new MixxxKeyEvent(data);
    MixxxDebug( evt.scan.toString(16)+"\t"+evt.modString()+"\t"+evt.keyCode()+"\t["+evt.typeString()+"]");
  }
}

MixxxKeyboardDevice.prototype.init = function(id){
  this.id = id;
  this.MixxxDebug("i'm a new MixxxKeyboardDevice, initialized. id = "+id);
}
MixxxKeyboardDevice.prototype.shutdown = function(){
  this.MixxxDebug("i'm the MixxxKeyboardDevice with id "+this.id+" and i'm shuttin' down.");
}
MixxxKeyboard = new MixxxKeyboardDevice();
Controller();

