
#include "engine/channelhandle.h"

bool operator == ( const ChannelHandle& h1, const ChannelHandle& h2)
{
  return h1.handle() == h2.handle();
}
bool operator != ( const ChannelHandle& h1, const ChannelHandle& h2)
{
  return h1.handle() != h2.handle();
}
QDebug operator<<(QDebug stream, const ChannelHandle& h)
{
  return (stream << "ChannelHandle("<<h.handle() << ")");
}
uint qHash(const ChannelHandle& h)
{
  return qHash(h.handle());
}
uint qHash(const ChannelHandleAndGroup& handle_group)
{
  return qHash(handle_group handle());
}
QDebug operator<<(QDebug stream, const ChannelHandleAndGroup& g)
{
    stream << "ChannelHandleAndGroup(" << g.name() << "," << g.handle() << ")";
    return stream;
}
bool operator==(const ChannelHandleAndGroup& g1, const ChannelHandleAndGroup& g2)
{
    return g1.handle() == g2.handle();
}

bool operator!=(const ChannelHandleAndGroup& g1, const ChannelHandleAndGroup& g2)
{
    return g1.handle() != g2.handle();
}
