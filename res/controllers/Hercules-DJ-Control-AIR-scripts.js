((function(){
    function HerculesAir () {
        this.beatStepDeckA1 = 0
        this.beatStepDeckA2 = 0x44
        this.beatStepDeckB1 = 0
        this.beatStepDeckB2 = 0x4C

        this.scratchEnable_alpha = 1.0/8
        this.scratchEnable_beta = (1.0/8)/32
        this.scratchEnable_intervalsPerRev = 128
        this.scratchEnable_rpm = 33+1/3

        this.shiftButtonPressed = false
        this.enableSpinBack = false

        this.wheel_multiplier = 0.4
    }
HerculesAir.prototype = {
    init : function(id) {
    this.id = id;

	// extinguish all LEDs
    for (var i = 79; i<79; i++) {
        midi.sendShortMsg(0x90, i, 0x00);
    }
	midi.sendShortMsg(0x90, 0x3B, 0x7f) // headset volume "-" button LED (always on)
	midi.sendShortMsg(0x90, 0x3C, 0x7f) // headset volume "+" button LED (always on)

	if(engine.getValue("[Master]", "headMix") > 0.5) {
		midi.sendShortMsg(0x90, 0x39, 0x7f) // headset "Mix" button LED
	} else {
		midi.sendShortMsg(0x90, 0x3A, 0x7f) // headset "Cue" button LED
	}

    // Set soft-takeover for all Sampler volumes
    for (var i=engine.getValue("[Master]","num_samplers"); i>=1; i--) {
        engine.softTakeover("[Sampler"+i+"]","pregain",true);
    }
    // Set soft-takeover for all applicable Deck controls
    for (var i=engine.getValue("[Master]","num_decks"); i>=1; i--) {
        engine.softTakeover("[Channel"+i+"]","volume",true);
        engine.softTakeover("[Channel"+i+"]","filterHigh",true);
        engine.softTakeover("[Channel"+i+"]","filterMid",true);
        engine.softTakeover("[Channel"+i+"]","filterLow",true);
    }

    engine.softTakeover("[Master]","crossfader",true);

	engine.connectControl("[Channel1]", "beat_active", "this.beatProgressDeckA")
	engine.connectControl("[Channel1]", "play", "this.playDeckA")

	engine.connectControl("[Channel2]", "beat_active", "this.beatProgressDeckB")
	engine.connectControl("[Channel2]", "play", "this.playDeckB")

    print ("Hercules DJ Controll AIR: "+id+" initialized.");
},
shutdown : function() {
	this.resetLEDs()
},

/* -------------------------------------------------------------------------- */
playDeckA : function() {
	if(engine.getValue("[Channel1]", "play") == 0) {
		// midi.sendShortMsg(0x90, this.beatStepDeckA1, 0x00)
		this.beatStepDeckA1 = 0x00
		this.beatStepDeckA2 = 0x44
	}
},

playDeckB : function() {
	if(engine.getValue("[Channel2]", "play") == 0) {
		// midi.sendShortMsg(0x90, HerculesAir.beatStepDeckB1, 0x00)
		this.beatStepDeckB1 = 0x00
		this.beatStepDeckB2 = 0x4C
	}
},
beatProgressDeckA : function() {
	if(engine.getValue("[Channel1]", "beat_active") == 1) {
		if(this.beatStepDeckA1 != 0x00) {
			midi.sendShortMsg(0x90, this.beatStepDeckA1, 0x00)
		}

		this.beatStepDeckA1 = this.beatStepDeckA2

		midi.sendShortMsg(0x90, this.beatStepDeckA2, 0x7f)
		if(this.beatStepDeckA2 < 0x47) {
			this.beatStepDeckA2++
		} else {
			this.beatStepDeckA2 = 0x44
		}
	}
},beatProgressDeckB : function() {
	if(engine.getValue("[Channel2]", "beat_active") == 1) {
		if(this.beatStepDeckB1 != 0) {
			midi.sendShortMsg(0x90, this.beatStepDeckB1, 0x00)
		}

		this.beatStepDeckB1 = this.beatStepDeckB2

		midi.sendShortMsg(0x90, this.beatStepDeckB2, 0x7f)
		if(this.beatStepDeckB2 < 0x4F) {
			this.beatStepDeckB2++
		} else {
			this.beatStepDeckB2 = 0x4C
		}
	}
}, headCue : function(midino, control, value, status, group) {
	if(engine.getValue(group, "headMix") == 0) {
		engine.setValue(group, "headMix", -1.0);
		midi.sendShortMsg(0x90, 0x39, 0x00);
		midi.sendShortMsg(0x90, 0x3A, 0x7f);
	}
},headMix : function(midino, control, value, status, group) {
	if(engine.getValue(group, "headMix") != 1) {
		engine.setValue(group, "headMix", 0);
		midi.sendShortMsg(0x90, 0x39, 0x7f);
		midi.sendShortMsg(0x90, 0x3A, 0x00);
	}
}, sampler : function(midino, control, value, status, group) {
	if(value != 0x00) {
		if(this.shiftButtonPressed) {
			engine.setValue(group, "LoadSelectedTrack", 1)
		} else if(engine.getValue(group, "play") == 0) {
			engine.setValue(group, "start_play", 1)
		} else {
			engine.setValue(group, "play", 0)
		}
	}
}, wheelTurn : function(midino, control, value, status, group) {
    var deck = script.deckFromGroup(group);
    var newValue=(value==0x01 ? 1: -1);
    // See if we're scratching. If not, do wheel jog.
    if (!engine.isScratching(deck)) {
        engine.setValue(group, "jog", newValue* HerculesAir.wheel_multiplier);
        return;
    }

    if (engine.getValue(group, "play") == 0) {
		var new_position = engine.getValue(group,"playposition") + 0.008 * (value == 0x01 ? 1 : -1)
		if(new_position<0) new_position = 0
		if(new_position>1) new_position = 1
		engine.setValue(group,"playposition",new_position);
    } else {
        // Register the movement
        engine.scratchTick(deck,newValue);
    }
},
jog : function(midino, control, value, status, group) {
    if (this.enableSpinBack) {
        this.wheelTurn(midino, control, value, status, group);
    } else {
        var deck = script.deckFromGroup(group);
        var newValue = (value==0x01 ? 1:-1);
        engine.setValue(group, "jog", newValue* this.wheel_multiplier);
    }
},
scratch_enable : function(midino, control, value, status, group) {
    var deck = script.deckFromGroup(group);
	if(value == 0x7f) {
		engine.scratchEnable(
			deck,
			this.scratchEnable_intervalsPerRev,
			this.scratchEnable_rpm,
			this.scratchEnable_alpha,
			this.scratchEnable_beta
		);
	} else {
		engine.scratchDisable(deck);
	}
}, shift : function(midino, control, value, status, group) {
	this.shiftButtonPressed = (value == 0x7f);
    midi.sendShortMsg(status, control, value);
}, spinback: function(midino, control, value, status,group) {
    if (value==0x7f) {
        this.enableSpinBack = !this.enableSpinBack;
        if (this.enableSpinBack) {
            midi.sendShortMsg(status,control, 0x7f);
        } else {
            midi.sendShortMsg(status,control, 0x0);
        }
    }
}
};
return HerculesAir;
})())
