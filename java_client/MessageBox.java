import java.awt.*;
import java.awt.event.*;

/**
 * Partially implemented message box.  The support for various buttons is supported,
 * but there is no support for icons or for different default button selections.
 * Maybe sometime later.
 * 
 * TODO: Fix the message box class to also work with an applet...
 */
public class MessageBox extends Dialog implements ActionListener { 

	/**
	 * Flags specifying the type of message box.
	 */
	public static final long OK					= 0x00000000;
	public static final long OKCANCEL			= 0x00000001;
	public static final long ABORTRETRYIGNORE	= 0x00000002;
	public static final long YESNOCANCEL		= 0x00000003;
	public static final long YESNO				= 0x00000004;
	public static final long RETRYCANCEL		= 0x00000005;
	
	/**
	 * Flags for the icon displayed on the lhs of the message box.  Currently unused.
	 */
	public static final long ICONHAND			= 0x00000010;
	public static final long ICONQUESTION		= 0x00000020;
	public static final long ICONEXCLAMATION	= 0x00000030;
	public static final long ICONASTERISK		= 0x00000040;
	public static final long ICONWARNING		= ICONEXCLAMATION;
	public static final long ICONERROR			= ICONHAND;
	public static final long ICONINFORMATION	= ICONASTERISK;
	public static final long ICONSTOP			= ICONHAND;
	
	/** 
	 * Flags for deciding the default button
	 */
	public static final long DEFBUTTON1			= 0x00000000;
	public static final long DEFBUTTON2			= 0x00000100;
	public static final long DEFBUTTON3			= 0x00000200;
	
	/**
	 * Masks for selecting individual parts of the flags
	 */
	protected static final long BUTTONMASK		= 0x0000000F;
	protected static final long ICONMASK		= 0x000000F0;
	protected static final long DEFMASK			= 0x00000F00;
	
	/**
	 * Return values
	 */
	public static final int IDOK				= 1;
	public static final int IDCANCEL			= 2;
	public static final int IDABORT				= 3;
	public static final int IDRETRY				= 4;
	public static final int IDIGNORE			= 5;
	public static final int IDYES				= 6;
	public static final int IDNO				= 7;

	/** 
	 * Label names
	 */
	protected static final String STROK			= "OK";
	protected static final String STRCANCEL		= "Cancel";
	protected static final String STRABORT		= "Abort";
	protected static final String STRRETRY		= "Retry";
	protected static final String STRIGNORE		= "Ignore";
	protected static final String STRYES		= "Yes";
	protected static final String STRNO			= "No";
	
	/**
	 * Member variables for the visual components
	 */
	protected MultiText messageLabel	= null;
	protected Frame mainFrame			= null;
	protected java.util.Vector buttons	= null;
	protected Frame parent				= null;

	/**
	 * Member variable which stores the return value to be passed to the 
	 * calling function.
	 */
	protected int returnValue			= IDOK;
	
	/**
	 * Constructor needs a parent frame to do a modal dialog.  This will _not_ work
	 * for an applet.
	 */
	public MessageBox(Frame parent) { 
		super(parent);
		this.parent = parent;
	}
	
	/**
	 * Display a message box and wait for the response.
	 */
	public int show(String message, String title, long flags) { 
		addButtons(flags & BUTTONMASK);
		messageLabel = new MultiText(parent, message);
		
		setTitle(title);
		setModal(true);
		//setResizable(false);
		setLayout(new BorderLayout());
		add(messageLabel, "Center");
			
		// Add the buttons at the bottom (south)
		GridBagLayout gbl = new GridBagLayout();
		Panel buttonPanel = new Panel(gbl);
		add(buttonPanel, "South");

		GridBagConstraints gbc = new GridBagConstraints();
		gbc.insets = new Insets(5, 5, 5, 5);
		gbc.fill = GridBagConstraints.NONE;
		gbc.gridwidth = 1;
		java.util.Enumeration buttonList = buttons.elements();
		while (buttonList.hasMoreElements()) { 
			Button button = (Button)buttonList.nextElement();
			gbl.setConstraints(button, gbc);
			buttonPanel.add(button);
		}

		pack();
		
		// resize to centre in application window
		Point     sP = parent.getLocationOnScreen();
		Rectangle pS = parent.getBounds();
		Rectangle wS = getBounds();
		setBounds(sP.x + (pS.width - wS.width)/2, sP.y + (pS.height - wS.height)/2, wS.width, wS.height);
		
		show();
		
		return getReturnValue();
	}
	
	/**
	 * Add all the buttons to the vector, dependent upon the flags for the buttons.
	 */
	protected void addButtons(long flags) { 
		Button newButton = null;
			
		buttons = new java.util.Vector();
			
		if ((flags == OK) || (flags == OKCANCEL)) { 
			// Add an OK button
			newButton = new Button(STROK);
			newButton.addActionListener(this);
			buttons.addElement(newButton);
		}
		if (flags == ABORTRETRYIGNORE) {
			// Add an abort button
			newButton = new Button(STRABORT);
			newButton.addActionListener(this);
			buttons.addElement(newButton);
		}
		if ((flags == ABORTRETRYIGNORE) || (flags == RETRYCANCEL)) { 
			// Add a retry button
			newButton = new Button(STRRETRY);
			newButton.addActionListener(this);
			buttons.addElement(newButton);
		}
		if (flags == ABORTRETRYIGNORE) {
			// Add an ignore button
			newButton = new Button(STRIGNORE);
			newButton.addActionListener(this);
			buttons.addElement(newButton);
		}
		if ((flags == YESNO) || (flags == YESNOCANCEL)) {
			// Add a yes and a no button
			newButton = new Button(STRYES);
			newButton.addActionListener(this);
			buttons.addElement(newButton);
			newButton = new Button(STRNO);
			newButton.addActionListener(this);
			buttons.addElement(newButton);
		}
		if ((flags == OKCANCEL) || (flags == YESNOCANCEL)
			|| (flags == RETRYCANCEL)) { 
			// Add a Cancel button
			newButton = new Button(STRCANCEL);
			newButton.addActionListener(this);
			buttons.addElement(newButton);
		}
	}

	/**
	 * Returns the value of the button pressed.
	 */
	public int getReturnValue() { 
		return returnValue;
	}
	
	/**
	 * Called when a button is pressed.  It identifies the button and sets the
	 * return value appropriately.  THen it hides the window, so the calling function
	 * is returned to.
	 */
	public void actionPerformed(ActionEvent e) { 
		
		if (e.getActionCommand().compareTo(STROK) == 0)
			returnValue = IDOK;
		else if (e.getActionCommand().compareTo(STRCANCEL) == 0)
			returnValue = IDCANCEL;
		else if (e.getActionCommand().compareTo(STRABORT) == 0)
			returnValue = IDABORT;
		else if (e.getActionCommand().compareTo(STRRETRY) == 0)
			returnValue = IDRETRY;
		else if (e.getActionCommand().compareTo(STRIGNORE) == 0)
			returnValue = IDIGNORE;
		else if (e.getActionCommand().compareTo(STRYES) == 0)
			returnValue = IDYES;
		else if (e.getActionCommand().compareTo(STRNO) == 0)
			returnValue = IDNO;
		else
			System.err.println("Unknown message source:  " + e.getActionCommand());
		
		setEnabled(false);
		return;
	}
}
