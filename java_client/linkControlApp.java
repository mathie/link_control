public class linkControlApp
{
	public static void main(String[] args)
	{
		usePageParams(args);
		
		java.awt.Frame mainFrame = new java.awt.Frame();
		mainFrame.setTitle(windowTitle);
		
		mainFrame.addWindowListener(new java.awt.event.WindowAdapter() {
            public void windowClosing(java.awt.event.WindowEvent e) {
                System.exit(0);
            }
        });
		try { 
			linkControl mainClass = new linkControl (mainFrame, multicastGroup, multicastPort);
		} catch (MulticastNotValidException e) { 
			MessageBox mb = new MessageBox(mainFrame);
			mb.show("Error:  Multicast address not valid:  " + e.getLocalizedMessage(), windowTitle, MessageBox.OK);
			System.exit(-1);
		}
		
		// Once the mainframe has been initialised, show it.
		mainFrame.show();
		mainFrame.pack();
		mainFrame.setResizable(false);
	}

	/**
	 * The Window title
	 */
	private static final String windowTitle         = linkControl.appTitle;
	
	/**
	 * Defaults for all user-configurable parameters
	 */
	private static       String multicastGroup      = "239.255.42.42";
	private static       int    multicastPort       = 6789;
	
	/**
	 * Reads parameters from the command line and set the correct
	 * properties.
	 */
	private static void usePageParams(String[] args)
	{
		String multicastGroupValue = null;
		String multicastPortValue = null;

		try { 
			multicastGroupValue = args[0];
			multicastPortValue  = args[1];
		}
		catch (ArrayIndexOutOfBoundsException e) { 
			/* didn't have enough command line parameters.  Doesn't matter. */
		}

		if (multicastGroupValue != null)
			multicastGroup = multicastGroupValue;
		
		if (multicastPortValue != null)
			multicastPort = Integer.getInteger(multicastPortValue).intValue();
	}
}
