public class linkClientStates {
	/**
	 * Enumeration of possible states
	 */
	public static final int INVALID      = -1;
	public static final int DISCONNECTED =  0;
	public static final int CONNECTED    =  1;
	public static final int UNKNOWN      =  2;
	/**
	 * String equivalents of the enumerations
	 */
	public static final String connectedString    = "Connected";
	public static final String disconnectedString = "Disconnected";
	public static final String unknownString	  = "Unknown";
	
	/**
	 * A neato function to turn a protocol string for the state into its numeric
	 * value
	 */
	public static int getValue(String state) { 
		if (state.compareTo(disconnectedString) == 0)
			return DISCONNECTED;
		else if (state.compareTo(connectedString) == 0)
			return CONNECTED;
		else if (state.compareTo(unknownString) == 0)
			return UNKNOWN;
		else
			return INVALID;
	}
	
	/**
	 * A neat function to turn the numeric value for the state into a string
	 */
	public static String toString (int state) { 
		switch (state) { 
		case CONNECTED:
			return connectedString;
		case DISCONNECTED:
			return disconnectedString;
		case UNKNOWN:
			return unknownString;
		default:
			return null;
		}
	}
	
	/**
	 * Check to see if the state is a valid one
	 */
	public static boolean valid (int state) {
		if ((state < 0) || (state > 2))
			return false;
		else
			return true;
	}
}
