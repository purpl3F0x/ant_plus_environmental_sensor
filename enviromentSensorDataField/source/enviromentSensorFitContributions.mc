import Toybox.FitContributor;
import Toybox.Lang;
import Toybox.Time;


class EnviromentSensorContributor {

	private enum FieldId {
        FIELD_CUR_TEMP 		= 0,
        FIELD_AVG_TEMP	 	= 1,
        FIELD_24HR_MIN 		= 2,
        FIELD_24HR_MAX	 	= 3,
        FIELD_CUR_HUM 		= 4,
        FIELD_CUR_PRESS 	= 5,
        FIELD_AVG_HUM 		= 6,
        FIELD_AVG_PRESS 	= 7,
        FIELD_BAT_STATUS    = 8   
    }
    
    const BatteryStatusString = ["--", "New", "Good", "Ok", "Low", "Critical", "Charging", "Invalid"];
    
    public const MESG_TYPE_DEVICE_INFO = 23;
    
//    public const MESG_DEVICE_INFO_BATTERY_STATUS = 11;
//    public const MESG_DEVICE_TIMESTAMP = 253;
    
    
    
    private var _curTemp as Float = null;
    private var _avgTemp as Float = null;
    private var _24hrMinTemp as Float = null;
    private var _24hrMaxTemp as Float = null;
    private var _curHum as Float = null;
    private var _curPress as Numeric = null;
    private var _avgHum as Float = null;
    private var _avgPress as Numeric = null;
    private var _batteryStatus as Numeric = 7;
 	
 	// Variables for computing averages   
    private var _temperatureRecordCount as Numeric = 0;
    private var _hum_pressRecordCount as Numeric = 0 ;
    private var _tempSessionAvg as Float = 0;
    private var _humSessionAvg as Float = 0;
    private var _pressSessionAvg as Numeric = 0;
    


    // FIT Contributions variables
    private var _curTempField as Field;
    private var _avgTempField as Field;
    private var _24hrMinTempField as Field;
    private var _24hrMaxTempField as Field;
    private var _curHumField as Field;
    private var _curPressField as Field;
    private var _avgHumField as Field;
    private var _avgPressField as Field;
    private var _batteryStatusField as Field;
//    private var _deviceInfoTimeStamp as Field;
    
    private var _recording as Bool = false;
    
    
    //! Constructor
    //! @param dataField Data field to use to create fields
    public function initialize(dataField as testcomplex_datafieldView) {
    	_curTempField = dataField.createField("curtemperature", FIELD_CUR_TEMP, FitContributor.DATA_TYPE_FLOAT, { :mesgType=>FitContributor.MESG_TYPE_RECORD, :units=> "C" });
    	_avgTempField = dataField.createField("avg_temperature", FIELD_AVG_TEMP, FitContributor.DATA_TYPE_FLOAT, { :mesgType=>FitContributor.MESG_TYPE_SESSION,:units=> "C" });
    	_24hrMinTempField = dataField.createField("24hrLowTemp", FIELD_24HR_MIN, FitContributor.DATA_TYPE_FLOAT, { :mesgType=>FitContributor.MESG_TYPE_SESSION, :units=> "C"});
    	_24hrMaxTempField = dataField.createField("24hrHighTemp", FIELD_24HR_MAX, FitContributor.DATA_TYPE_FLOAT, { :mesgType=>FitContributor.MESG_TYPE_SESSION, :units=> "C"});
    	
    	_curHumField = dataField.createField("curHumidity", FIELD_CUR_HUM, FitContributor.DATA_TYPE_FLOAT, { :nativeNum=>93, :mesgType=>FitContributor.MESG_TYPE_RECORD, :units=> "%" });
    	_curPressField = dataField.createField("absolute_pressure", FIELD_CUR_PRESS, FitContributor.DATA_TYPE_UINT32, { :mesgType=>FitContributor.MESG_TYPE_RECORD, :units=> "Pa" });
    	
    	_avgHumField = dataField.createField("avgHumidity", FIELD_AVG_HUM, FitContributor.DATA_TYPE_FLOAT, { :mesgType=>FitContributor.MESG_TYPE_SESSION, :units=> "%" });
    	_avgPressField = dataField.createField("avgPress", FIELD_AVG_PRESS, FitContributor.DATA_TYPE_UINT32, { :mesgType=>FitContributor.MESG_TYPE_SESSION, :units=> "Pa" });
    	
    	_batteryStatusField = dataField.createField("battery_status", FIELD_BAT_STATUS, FitContributor.DATA_TYPE_STRING, {:mesgType=> FitContributor.MESG_TYPE_SESSION, :count => 10});
    	
    }
    
    
    //! Update data and fields
    //! @param sensor The ANT+ channel and data
    public function compute(sensor as EnviromentSensor) {
    	if (sensor == null){ return; }
    	if (sensor.isPaired() == false){ return; }
    	
    	var curTemp = sensor.getTemperature();
    	var curHum = sensor.getHumidity();
    	var curPress = sensor.getPressure();
    	
    	
    	if (_curTemp != curTemp) {
    		_curTemp = curTemp;
    		_curTempField.setData(curTemp);
    	}
    	
    	if (_curHum != curHum){
    		_curHum = curHum;
    		_curHumField.setData(curHum);
    	}
    	
    	if (_curPress != curPress) {
    		_curPress = curPress;
    		_curPressField.setData(curPress);
    	}
    	
    	
    	if (curTemp != null) {
    		_tempSessionAvg += curTemp;
    		_temperatureRecordCount++;
    		_avgTempField.setData( _tempSessionAvg / _temperatureRecordCount);
    	}
    	
    	if (curHum != null && curPress != null) {
    		_pressSessionAvg += curPress;
    		_humSessionAvg += curHum;
    		_hum_pressRecordCount++;
    		_avgHumField.setData( _humSessionAvg / _hum_pressRecordCount);
    	    _avgPressField.setData( _pressSessionAvg / _hum_pressRecordCount);
    	}
    	
    }
    
    //! Update data and fields
    //! @param sensor The ANT+ channel and data
    public function onStop(sensor as EnviromentSensor) { 
    	if (sensor == null){ return; }
    	
    	_batteryStatusField.setData(BatteryStatusString[sensor.getBatteryStatus()]);
    }
}



