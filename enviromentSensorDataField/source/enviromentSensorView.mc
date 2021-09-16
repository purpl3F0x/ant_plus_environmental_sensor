import Toybox.Activity;
import Toybox.Application;
import Toybox.Graphics;
import Toybox.Lang;
import Toybox.System;
import Toybox.WatchUi;


class EnviromentSensorDataFieldView extends WatchUi.DataField {
    
    private var _sensor as EnviromentSensor?;
    
   	// Fit Contributor
    private var _fitContributor as EnviromentSensorContributor;
        

    function initialize(sensor as EnviromentSensor?) {
        DataField.initialize();        
        _sensor = sensor;
        _fitContributor = new EnviromentSensorContributor(self);
        
    }

    // Set your layout here. Anytime the size of obscurity of
    // the draw context is changed this will be called.
    function onLayout(dc as Dc) as Void {
        var obscurityFlags = DataField.getObscurityFlags();
        

        View.setLayout(Rez.Layouts.MainLayout(dc));
        var labelView = View.findDrawableById("tempLabel");
        labelView.locY = labelView.locY - 16;     
        var valueView = View.findDrawableById("tempValue");
        valueView.locY = valueView.locY + 7;
        
        labelView = View.findDrawableById("humLabel");
        labelView.locY = labelView.locY - 16;     
        valueView = View.findDrawableById("humValue");
        valueView.locY = valueView.locY + 7;
        
        
        labelView = View.findDrawableById("pressLabel");
        labelView.locY = labelView.locY - 16;     
        valueView = View.findDrawableById("pressValue");
        valueView.locY = valueView.locY + 7;
            

        (View.findDrawableById("tempLabel") as Text).setText(Rez.Strings.tempLabelShort);
        (View.findDrawableById("humLabel") as Text).setText(Rez.Strings.humLabelShort);
        (View.findDrawableById("pressLabel") as Text).setText(Rez.Strings.pressLabelShort);
        
        
    }

    // The given info object contains all the current workout information.
    // Calculate a value and save it locally in this method.
    // Note that compute() and onUpdate() are asynchronous, and there is no
    // guarantee that compute() will be called before onUpdate().
    function compute(info as Activity.Info) as Void {
    	_fitContributor.compute(_sensor);
    }
    
    //! Handle the activity timer stopping
    public function onTimerStop() as Void {
        _fitContributor.onStop(_sensor);
    }

    //! Handle an activity timer pause
    public function onTimerPause() as Void {
//        System.println("Activity Paused");
        _fitContributor.onStop(_sensor);
    }

    // Display the value you computed here. This will be called
    // once a second when the data field is visible.
    function onUpdate(dc as Dc) as Void {
        

        // Set the foreground color and value
        var tempValue = View.findDrawableById("tempValue") as Text;
        var humValue = View.findDrawableById("humValue") as Text;
		var pressValue = View.findDrawableById("pressValue") as Text;
        
        if (getBackgroundColor() == Graphics.COLOR_BLACK) {
            tempValue.setColor(Graphics.COLOR_WHITE);
            humValue.setColor(Graphics.COLOR_WHITE);
            pressValue.setColor(Graphics.COLOR_WHITE);
        } else {
            tempValue.setColor(Graphics.COLOR_BLACK);
            humValue.setColor(Graphics.COLOR_BLACK);
            pressValue.setColor(Graphics.COLOR_BLACK);
            
        }
        
        if (_sensor.isPaired()){
	        tempValue.setText(_sensor.getTemperature().format("%.1f"));
	        humValue.setText(_sensor.getHumidity().format("%.1f"));
	        pressValue.setText(_sensor.getPressureInKPa().format("%.1f"));
	    } else {
	    	tempValue.setText("--");
	        humValue.setText("--");
	        pressValue.setText("--");
	    }
        
 
        // Call parent's onUpdate(dc) to redraw the layout
        View.onUpdate(dc);
    }
    
    
    public function onSensorConnected() {
//    	System.println("tBattery  Status: ");
    	return;
    }

}



