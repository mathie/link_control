import java.awt.*;
import java.applet.*;

/**
 * This class reads PARAM tags from its HTML host page and passes
 * them into the main class which is shared between this and the
 * application version. Program execution begins with the init() method. 
 */
public class linkControlApplet extends Applet
{
	/**
	 * The entry point for the applet. 
	 */
	public void init()
	{
		usePageParams();
	
		try {
			linkControl mainClass = new linkControl (this, multicastGroup, multicastPort);
		} catch (MulticastNotValidException e) { 
			// TODO: Fix this!  The user will never see such an error message.  Create a dialog to do it instead.
			System.err.println("Error:  Multicast address not valid:  " + e.getLocalizedMessage());
			System.exit(-1);
		}
	}

	/**
	 * Parameter names
	 */
	private final String multicastGroupParam = "multicastGroup";
	private final String multicastPortParam  = "multicastPort";
	
	/**
	 * Default parameter values
	 */
	private       String multicastGroup      = "239.255.42.42";
	private       int    multicastPort       = 6789;
	
	/**
	 * Reads parameters from the applet's HTML host and sets applet
	 * properties.
	 */
	private void usePageParams()
	{
		String multicastGroupValue;
		String multicastPortValue;
		/** 
		 * Read the <PARAM NAME="multicast_group" VALUE="w.x.y.z">,
		 * and <PARAM NAME="multicast_port" VALUE="n"> tags from
		 * the applet's HTML host.
		 */
		multicastGroupValue = getParameter (multicastGroupParam);
		multicastPortValue  = getParameter (multicastPortParam);

		if (multicastGroupValue != null)
			multicastGroup = multicastGroupValue;
		
		try {
			if (multicastPortValue != null)
				multicastPort = Integer.valueOf(multicastPortValue).intValue();
		} catch (NumberFormatException e) { 
			// If the parameter is unintelligable, just use the default
		}
	}

	/**
	 * External interface used by design tools to show properties of an applet.
	 */
	public String[][] getParameterInfo()
	{
		String[][] info =
		{
			{ multicastGroupParam, "String", "IP address of the multicast group" },
			{ multicastPortParam,  "String", "Port number to listen on" },
		};
		return info;
	}
}
