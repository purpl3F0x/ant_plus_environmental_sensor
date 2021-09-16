import Toybox.Ant;
import Toybox.AntPlus;
import Toybox.Lang;
import Toybox.Time;
import Toybox.FitContributor;

class EnviromentSensor extends Ant.GenericChannel {

    private var _chanAssign as ChannelAssignment;
    private var _deviceCfg as DeviceConfig;
    private var _isSearching as Boolean;
	
	// Page 1 Data
	private var _pastEventCount as Number;    
    private var _temp as Float = 0;
    private var _24hMaxTemp = null, _24hMinTemp = null as Float;
    
    // Page 84 Data
    private var _hum = 0 as Float;			// in %Rh
    private var _press = 0 as Numeric;		// in Pa
    
    // Page 82 Data
    private var _batteryStatus = 0 as AntPlus.BatteryStatusValue;
    private var _batteryVoltage as Float;
    private var _operatingTime as Numeric;
    

    //! Constructor
    public function initialize() {
        // Get the channel
        _chanAssign = new Ant.ChannelAssignment(
            Ant.CHANNEL_TYPE_RX_NOT_TX,
            Ant.NETWORK_PLUS
        );
        GenericChannel.initialize(method(: onMessage), _chanAssign);

        // Set the configuration
        _deviceCfg = new Ant.DeviceConfig({
            : deviceNumber => 0, 				// Wildcard our search
            : deviceType => 25, 				// Ant+ Enviroment Sensor
            : transmissionType => 0, 			// Wildcard Search
            : messagePeriod => 8192, 			// 4 Hz
            : radioFrequency => 57, 			// Ant+ Frequency (2400 + 57Mhz)
            : searchTimeoutLowPriority => 4, 	// Timeout in 4 * 2.5 = 10s,
            : searchThreshold => 0
        }); // Pair to all transmitting sensors

        GenericChannel.setDeviceConfig(_deviceCfg);

        _isSearching = true;        
        
    }

    //! Open an ANT channel
    //! @return true if channel opened successfully, false otherwise
    public function open() as Boolean {
        var open = GenericChannel.open();
        _pastEventCount = 0;
        _isSearching = true;
        return open;
    }
    
    
    //! Close the ANT sensor and save the session
    public function close() as Void {
    	_isSearching = true;
        GenericChannel.close();
    }
    
    function release() {
    	GenericChannel.release();
    }
    
    
    //! Check if device is connected to a sensor
    //! @return true if a sensor is connected 
    public function isPaired() as Bool {
    	return !_isSearching;
    }
    
    //! Get Current Temperature
    public function getTemperature() as Float {
    	return _temp;
    }
    
    //! Get Current Humidity
    public function getHumidity() as Float {
    	return _hum;
    }
    
    //! Get Current Barrometric Pressure
    public function getPressure() as Numeric {
    	return _press;
    }
    
    //! Get Current Barrometric Pressure in kPa
    public function getPressureInKPa() as Float {
    	return _press * 0.001f;
    }
    
    //! Get Battery Status Enum
    public function getBatteryStatus() as AntPlus.BatteryStatusValue {
     	return _batteryStatus;
    }
    
    //! Update when a message is received
    //! @param msg The ANT message
    function onMessage(msg as Message) {
        // Parse the payload
        var payload = msg.getPayload();

        if( Ant.MSG_ID_BROADCAST_DATA == msg.messageId ) {
            
            // Were we searching?
            if(_isSearching) {
                _isSearching = false;
                Attention.playTone(Attention.TONE_LOUD_BEEP);
                // Update  device config primarily to see the device number of the sensor we paired to
                _deviceCfg = GenericChannel.getDeviceConfig();
                
                // Request Battery Info on Connect (2 times)
                var message = new Ant.Message();
    			message.setPayload([70, 0xFF, 0xFF, 0xFF, 0xFF, 2, 82, 1]);
    			GenericChannel.sendBroadcast(message);
    		}
            
			parseData(payload);
			
       	} // end broadcast data
        
        else if( Ant.MSG_ID_CHANNEL_RESPONSE_EVENT == msg.messageId ) {
            if( Ant.MSG_ID_RF_EVENT == (payload[0] & 0xFF) ) {
                if( Ant.MSG_CODE_EVENT_CHANNEL_CLOSED == (payload[1] & 0xFF) ) {
                    open();
                }
                else if( Ant.MSG_CODE_EVENT_RX_FAIL_GO_TO_SEARCH  == (payload[1] & 0xFF) ) {
                    _isSearching = true;
                }
            }
            else{
                //It is a channel response.
            }
        } // end channel response event

    } // end on message
    
    
    //! Pases the incoming broadcast data
    //! @param data The ANT message payload
    private function parseData(data as Lang.Array<Lang.Number>) {
    	var pageNumber = data[0];
    	
//    	System.println("Got Page: "  + pageNumber);
    	
    	switch (pageNumber) {
    		// General Information
    		case 0:
    			break;
    		
    		// Temperature Data
    		case 1:
    			_temp = .01f * (((data[7] & 0xFF) << 8) | (data[6] & 0xFF));
    			var tmp_24h_max = ((data[5] & 0xFF) << 4) | (data[4] & 0xF);
    			var tmp_24h_min = ((data[4] & 0xF) << 8) | (data[3] & 0xFF);
    			
    			if (tmp_24h_max != 0x8000) {
    				_24hMaxTemp = .1f * tmp_24h_max;
    			}
    			
    			if (tmp_24h_min != 0x8000) {
    				_24hMinTemp = .1f * tmp_24h_min;
    			}
    				
//    			System.println("\tTemperature Data:");
//    			System.println("\t\tTemperature:" + _temp);
//    			System.println("\t\t24H Min:" + _24hMinTemp);
//    			System.println("\t\t24H Max:" + _24hMaxTemp);
//  
    			break;
    			
    		// Manufacturer�s Identification
    		case 80:
    		// Product Information
    		case 81:
    			break; // lol
    		
    		// Battery Status
    		case 82:
    			_batteryStatus = (data[7] >> 4) & 0x7;
    			
    			_operatingTime =  (data[5] << 16) | (data[4] << 8) | data[3];
    			if (data[7] >> 7 == 1) {
    				_operatingTime *= 2;
    			} else {
    				_operatingTime *= 16;
    			}
    			
    			_batteryVoltage = (data[7] & 0x0F) + (data[6] / 256.0f);
    			
//    			System.println("\tBattery  Status: ");
//    			System.println("\t\tOperating Time " + _operatingTime);
//    			System.println("\t\tBattery  Voltage: " + _batteryVoltage);
//    			System.println("\t\tBattery  Status: " + _batteryStatus);
   			
    			break;
    		
    		// Subfield Data			
    		case 84:
    			for (var i=2; i < 4; i++) {
    				switch(data[i]){
    					// Barometric Pressure
    					case 2:
    						_press = ((data[i*2 + 1] << 8 ) | data[i*2]) * 10 ;
    						break;
    					
    					// Humidity
    					case 3:
    						_hum = ((data[i*2 + 1] << 8 ) | data[i*2]) * 0.01f;
    						break;
    				}
    			}
    			
    			break;
    		
    	}

    }
    
    
}    
    