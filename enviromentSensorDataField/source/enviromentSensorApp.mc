import Toybox.Application;
import Toybox.Lang;
import Toybox.WatchUi;
import Toybox.Ant;

class EnviromentSensorApp extends Application.AppBase {

	private var _sensor as EnviromentSensor?;
	

    function initialize() {
        AppBase.initialize();
    }

    // onStart() is called on application start up
    function onStart(state as Dictionary?) as Void {
    	try {
            // Create the sensor object and open it
            _sensor = new EnviromentSensor();
            (_sensor as EnviromentSensor).open();
        } catch (e instanceof Toybox.Ant.UnableToAcquireChannelException) {
            System.println(e.getErrorMessage());
            _sensor = null;
        }
    }

    // onStop() is called when your application is exiting
    function onStop(state as Dictionary?) as Void {
         if (_sensor != null) {
            _sensor.close();
         }
    }

    //! Return the initial view of your application here
    function getInitialView() as Array<Views or InputDelegates>? {
        return [ new EnviromentSensorDataFieldView(_sensor) ] as Array<Views or InputDelegates>;
    }

}

function getApp() as EnviromentSensorApp_ {
    return Application.getApp() as EnviromentSensorApp_;
}