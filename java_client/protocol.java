/**
 * This set of constant strings specifies the full protocol
 */
public class protocol { 
	 /**
	  * Notification peer
	  */
	public static final String notifyPrefix					= "NOTIFY";
	public static final String notifyIsUp					= "ISUP";
	public static final String notifyIsDown					= "ISDOWN";
	
	/**
	 * Client
	 */
	public static final String clientPrefix					= "CLIENT";
	public static final String clientPing					= "PING";
	public static final String clientDevices				= "DEVICES";
	public static final String clientUp						= "UP";
	public static final String clientDown					= "DOWN";
	public static final String clientForceDown				= "FORCE_DOWN";
	public static final String clientStatus					= "STATUS";
	public static final String clientClientStatus			= "CLIENT_STATUS";
	
	/**
	 * Server
	 */
	public static final String serverPrefix					= "SERVER";
	public static final String serverDevices				= "DEVICES";
	public static final String serverStatus					= "STATUS";
	public static final String serverStatusUp				= "UP";
	public static final String serverStatusDown				= "DOWN";
	public static final String serverStatusConnecting		= "CONNECTING";
	public static final String serverStatusDisconnecting	= "DISCONNECTING";
	public static final String serverClientStatus			= "CLIENT_STATUS";

	/**
	 * Server broadcast messages
	 */
	public static final String broadcastPrefix				= "BROADCAST";
	public static final String broadcastInit				= "INIT";
	public static final String broadcastStatus				= "STATUS";
	public static final String broadcastQuit				= "QUIT";
	
	public static final int    maxPacketSize				= 400; // bytes
}
