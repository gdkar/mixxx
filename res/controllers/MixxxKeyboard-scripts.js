
MixxxKeyboardDevice = function(){
    var MixxxDebug = this.MixxxDebug   = function MixxxDebug(msg){print("[MixxxKeyboard] "+msg);}
    var MixxxKeyEvent = this.MixxxKeyEvent = function MixxxKeyEvent(data){
    this.keycode   = data[0] + (data[1]<<8) + (data[2]<<16);
    this.modifiers = data[3];
    this.type      = data[4] + (data[5]<<8) + (data[6]<<16) + (data[7]<<24);
    this.typestring= "";
    if (this.type==6)this.typestring="Press";
    else if(this.type==7)this.typestring="Release";
  }
  this.incomingData = function IncomingData(data,len){
    var evt = new MixxxKeyEvent(data);
    MixxxDebug(evt.keycode+" "+evt.typestring);
  }
}

MixxxKeyboardDevice.prototype.init = function(id){
  this.id = id;
  this.MixxxDebug("i'm a now MixxxKeyboardDevice, initialized. id = "+id);
}
MixxxKeyboardDevice.prototype.shutdown = function(){
  this.MixxxDebug("i'm the MixxxKeyboardDevice with id "+this.id+" and i'm shuttin' down.");
}
MixxxKeyboard = new MixxxKeyboardDevice();
Controller();

