public class linkDeviceStates { 
	
	/**
	 * Enumeration of possible states
	 */
	public static final int INVALID       = -1;
	public static final int DOWN          =  0;
	public static final int UP            =  1;
	public static final int CONNECTING    =  2;
	public static final int DISCONNECTING =  3;
	public static final int UNKNOWN       =  4;
	
	/**
	 * String equivalents of the enumerations
	 */
	public static final String downString			= "Down";
	public static final String upString				= "Up";
	public static final String connectingString		= "Connecting";
	public static final String disconnectingString	= "Disconnecting";
	public static final String unknownString		= "Unknown";

	/**
	 * A neato function to turn a protocol string for the state into its numeric
	 * value
	 */
	public static int getValue(String state) { 
		if (state.compareTo(protocol.serverStatusDown) == 0)
			return DOWN;
		else if (state.compareTo(protocol.serverStatusUp) == 0)
			return UP;
		else if (state.compareTo(protocol.serverStatusConnecting) == 0)
			return CONNECTING;
		else if (state.compareTo(protocol.serverStatusDisconnecting) == 0)
			return DISCONNECTING;
		else
			return INVALID;
	}
	
	/**
	 * A neat function to turn the numeric value for the state into a string
	 */
	public static String toString (int state) { 
		switch (state) { 
		case DOWN:
			return downString;
		case UP:
			return upString;
		case CONNECTING:
			return connectingString;
		case DISCONNECTING:
			return disconnectingString;
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
		if ((state < 0) || (state > 4))
			return false;
		else
			return true;
	}
}
