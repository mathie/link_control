import java.awt.*;
import java.awt.event.*;

public class linkControl { 
	
	/**
	 * Constructor creates the main panel 
	 */
	public linkControl (Container panel, String multicastGroupStr, int multicastPort) throws MulticastNotValidException { 
		// Keep a local reference to the main panel we render the user interface onto
		mainPanel = panel;
		
		// Initialise a new list for holding server information
		linkServers = new linkServerList();

		// Render the user interface
		initForm();
		initListeners();
		
		// Setup the multicast socket that listens for information from servers
		try { 
			java.net.InetAddress multicastGroup = java.net.InetAddress.getByName(multicastGroupStr);
			multicastSocket = new java.net.MulticastSocket(multicastPort);
			multicastSocket.joinGroup(multicastGroup);
			System.err.println("Joined the multicast group");
		} catch (SecurityException e) {
			System.err.println("Not given permission to access multicast sockets!");
			throw new MulticastNotValidException(e.getLocalizedMessage());
		} catch (java.io.IOException e) { 
			throw new MulticastNotValidException(e.getLocalizedMessage());
		}
		
		// Start the thread which listens for information from the servers
		listenerThread = new multicastThread(multicastSocket, linkServers);
		listenerThread.start();
		
		// Start the thread which ages old servers
		timeoutThread = new ageThread(linkServers);
		timeoutThread.start();
	}
	
	/**
	 * ActionListener class which responds to actions performed when the connect
	 * button is clicked.
	 */
	protected class connectButtonListener implements ActionListener { 
		/**
		 * actionPerformed is called whenever the connect button is clicked.  The connect
		 * button should only be enabled if the client is not connected to the device.
		 * The function requests that the server connect the client to the device.
		 */
		public void actionPerformed (ActionEvent e) { 
			linkDevice device = getSelectedDevice();
			if (device == null)
				return;
			
			try {
				device.Connect();
			} catch (NoSuchServerException ex) { 
				// The server appears to have died.  Remove it from the list and refresh.
				linkServer server = getSelectedServer();
				if (server == null) { 
					// how can that happen?  No server selected?
					return;
				}
				try {
					linkServers.remove(server);
				} catch (NoSuchServerException ex2) { 
					// This cannot happen
					throw new Error(ex2.getMessage());
				}
			}
		}
	}
	
	/**
	 * ActionListener class which responds to actions performed on the disconnect
	 * button.
	 */
	protected class disconnectButtonListener implements ActionListener { 
		/**
		 * actionPerformed is called whenever the disconnect button is clicked.  Since the
		 * disconnect button is only available when the client is currently connected to the
		 * device, it calls the device's disconnect function.
		 */
		public void actionPerformed (ActionEvent e) { 
			linkDevice device = getSelectedDevice();
			if (device == null)
				return;

			try {
				device.Disconnect();
			} catch (NoSuchServerException ex) { 
				// The server appears to have died.  Remove it from the list and refresh.
				linkServer server = getSelectedServer();
				if (server == null) { 
					// how can that happen?  No server selected?
					return;
				}
				try {
					linkServers.remove(server);
				} catch (NoSuchServerException ex2) { 
					// This cannot happen
					throw new Error(ex2.getMessage());
				}
			}
		}
	}
	
	/**
	 * Observer class which monitors the serverList for changes.
	 */
	protected class serverListObserver implements java.util.Observer {
		/**
		 * update is called whenever the dataset that is being observed is
		 * modified.  It then modifies the current list view.
		 */
		public void update(java.util.Observable o, Object arg) {
			updateServerList();
		}
		
		/**
		 * Updates the server list to match the list of servers we know about.
		 */
		protected void updateServerList() { 
			// Since it's to be completely refreshed, destroy existing data
			serverList.removeAll();
			deviceList.removeAll();
			clearDeviceStatus();
			
			java.util.Enumeration names = linkServers.getServerNames();
			while (names.hasMoreElements()) { 
				serverList.add((String)names.nextElement());
			}
			
			// If there are items in the list, select the first one.
			if (serverList.getItemCount() > 0) {
				serverList.select(0);
				serverListListener.itemStateChanged(null);
			}
		}	
	}
	
	/**
	 * ItemListener listens for selection changes on the deviceList control.
	 */
	protected class deviceListItemListener implements ItemListener {
		/**
		 * Observer class monitors the currently displayed device for changes.
		 */
		protected class deviceObserver implements java.util.Observer {
			/**
			 * update is called whenever the data in the currently displayed device
			 * is changed and the class requests that changes be displayed.
			 */
			public void update(java.util.Observable o, Object arg) {
				updateCurrentDevice();
			}
		}
		
		/**
		 * The one and only instantiation of the deviceObserver class.
		 */
		protected deviceObserver observer = new deviceObserver();
		
		/**
		 * This class keeps track of the previous selection in order to remove 
		 * our Observer from it.
		 */
		protected linkDevice currentSelection = null;
		
		/**
		 * itemStateChanged is called when a device selection changes
		 */
		public void itemStateChanged (ItemEvent e) {
			// Remove the observer from the old selection and (if there is a new
			// selection) add it to the new one.
			updateCurrentDeviceObserver();
			
			// Display the new device's data.
			updateCurrentDevice();
		}
		
		/**
		 * Removes the observer from the previous selection and (if there is a new selection),
		 * adds it to the new one.
		 */
		protected void updateCurrentDeviceObserver() {
			// Remove the old observer if we had a previous selection
			if (currentSelection != null)
				currentSelection.deleteObserver(observer);
				
			// Get the new selection
			currentSelection = getSelectedDevice();
			
			// if there is a new selection, add an observer to it
			if (currentSelection != null)
				currentSelection.addObserver(observer);
		}

		/**
		 * Updates the controls displaying information about the device.
		 */
		protected void updateCurrentDevice() { 
			// Check see if there is a selection.  If not, simply return.
			if (currentSelection == null)
				return;
				
			// set the description, device status, number of users and connect time
			deviceDescription.setText(currentSelection.getDescription());
			deviceStatus.setText(linkDeviceStates.toString(currentSelection.getDeviceStatus()));
			usersConnected.setText(Long.toString(currentSelection.getNumUsers()));
			timeConnected.setText(Timespan.toString(currentSelection.getConnectTime()));
			
			// Show a textual representation of the client's status
			int CurrentClientStatus = currentSelection.getClientStatus();
			clientStatus.setText(linkClientStates.toString(CurrentClientStatus));
			
			// And enable the appropriate buttons.
			switch (CurrentClientStatus) { 
			case linkClientStates.CONNECTED:
				connectButton.setEnabled(false);
				disconnectButton.setEnabled(true);
				break;
			case linkClientStates.DISCONNECTED:
				connectButton.setEnabled(true);
				disconnectButton.setEnabled(false);
				break;
			default: // unknown state
				connectButton.setEnabled(true);
				disconnectButton.setEnabled(true);
			}
		}
	}
	
	protected class serverListItemListener implements ItemListener {
		/**
		 * Observer class which monitors the currently displayed deviceList for changes.
		 */
		protected class deviceListObserver implements java.util.Observer {
			/**
			 * update is called whenever the dataset that is being observed is
			 * modified.  It then modifies the current list view.
			 */
			public void update (java.util.Observable o, Object arg) {
				updateDeviceList();
			}
		}

		/**
		 * The one and only instantiation of the deviceListObserver class.
		 */
		protected deviceListObserver observer = new deviceListObserver();
		
		/**
		 * This class keeps track of the previous selection in order to remove 
		 * our Observer from it.
		 */
		protected linkDeviceList currentSelection = null;
	
		/**
		 * itemStateChanged is called when a device selection changes
		 */
		public void itemStateChanged (ItemEvent e) {
			// Remove the observer from the old selection and (if there is a new
			// selection) add it to the new one.
			updateCurrentDeviceListObserver();
			
			// Display the new device list.
			updateDeviceList();
		}
		
		/**
		 * Removes the observer from the previous selection and (if there is a new selection),
		 * adds it to the new one.
		 */
		protected void updateCurrentDeviceListObserver() {
			if (currentSelection != null)
				currentSelection.deleteObserver(observer);
				
			linkServer currentServer = getSelectedServer();
			if (currentServer != null) { 
				currentSelection = currentServer.getDevicesControlled();
				if (currentSelection != null)
					currentSelection.addObserver(observer);
			} else {
				currentSelection = null;
			}
		}
		
		/**
		 * Updates the device list to match the list of devices for the current server.
		 */
		protected void updateDeviceList() {
			// About to chenge the current server selection, so clear the device list
			deviceList.removeAll();
			clearDeviceStatus();
				
			if (currentSelection == null)
				return;
				
			java.util.Enumeration names = currentSelection.getDeviceNames();
			while (names.hasMoreElements()) { 
				deviceList.add((String)names.nextElement());
			}
			
			// If there are items in the list, select the first one.
			if (deviceList.getItemCount() > 0) {
				deviceList.select(0);
				deviceListListener.itemStateChanged(null);
			}
		}
	}
	
	/**
	 * The link control creates the multicast socket to pass to the listener thread
	 */
	protected java.net.MulticastSocket multicastSocket = null;
	
	/**
	 * Keep a reference to the thread which listens for multicast information from the
	 * server.  Primarily, this will be used to stop the thread when the rest of the 
	 * app wishes to quit.
	 */
	protected multicastThread listenerThread = null;
	
	/**
	 * Keep a reference to the thread which ages out old servers.
	 */
	protected ageThread timeoutThread = null;
	
	/**
	 * This is the master list of servers that the client knows about.
	 */
	protected linkServerList linkServers = null;
	
	/**
	 * The sole instantiations of the Observer which watches the server list
	 */
	protected serverListObserver linkServersObserver = new serverListObserver();
	
	/**
	 * The listeners which watch for selection changes on the two lists.
	 */
	protected serverListItemListener serverListListener = new serverListItemListener();
	protected deviceListItemListener deviceListListener = new deviceListItemListener();
	
	/**
	 * Labels on the main form.
	 */
	protected Label serverLabel				= new Label("Internet Server");
	protected Label deviceLabel				= new Label("Modem");
	protected Label statusLabel				= new Label("Status");
	protected Label deviceDescriptionLabel	= new Label("Description");
	protected Label clientStatusLabel		= new Label("My Status");
	protected Label deviceStatusLabel		= new Label("Modem Status");
	protected Label usersConnectedLabel		= new Label("Users Connected");
	protected Label timeConnectedLabel		= new Label("Uptime");

	/**
	 * The two lists contained on the main form.
	 */
	protected List serverList				= new List();
	protected List deviceList				= new List();
	
	/**
	 * All the buttons on the main form
	 */
	protected Button connectButton			= new Button("Connect");
	protected Button disconnectButton		= new Button("Disconnect");
	
	/**
	 * The text boxes on the main form, along with their default width.
	 */
	protected TextField clientStatus		= new TextField(40);
	protected TextField deviceStatus		= new TextField(40);
	protected TextField deviceDescription	= new TextField(40);
	protected TextField usersConnected		= new TextField(03);
	protected TextField timeConnected		= new TextField(10);

	/**
	 * String constants
	 */
	protected static final String connectMessage	= "This will connect the modem to the Internet.";
	protected static final String disconnectMessage = "This will close the Internet connection.";
	protected static final String appTitle			= "Link Controller";

	/**
	 * The main panel from which the form is constructed.
	 */
	protected Container mainPanel			= null; // Initialised by the constructor
	
	/**
	 * Utility function to return the linkServer of the currently selected server (according
	 * to the list box).  Returns null if there is no selection.
	 */
	protected linkServer getSelectedServer() { 
		try {
			String serverName = serverList.getItem(serverList.getSelectedIndex());
			return linkServers.getServer(java.net.InetAddress.getByName(serverName));
		} catch (NoSuchServerException ex) { 
			// The server has been removed between refreshes.  Refresh the
			// list to make sure it is up-to-date.
			// TODO: Let the user know what just happened
			linkServersObserver.update(null, null);
			return null;
		} catch (java.net.UnknownHostException ex) { 
			// Same problem as above
			linkServersObserver.update(null, null);
			return null;
		} catch (ArrayIndexOutOfBoundsException e) { 
			// Looks like the serverList has no selection
			return null;
		}
	}
	
	/**
	 * Utility function to return the linkDevice of the currently selected device (according
	 * to the list box).  Returns null if there is no selection.
	 */
	protected linkDevice getSelectedDevice() {
		linkServer server = getSelectedServer();
		if (server == null)
			return null;
		
		try {
			String deviceName = deviceList.getItem(deviceList.getSelectedIndex());
			return server.getDevicesControlled().getDevice(deviceName);
		} catch (NoSuchDeviceException ex) {
			// The device has disappeared between refreshes. Unlikely, but could
			// happen.
			// TODO: let the user know what's just happened
			linkServersObserver.update(null, null);
			return null;
		} catch (ArrayIndexOutOfBoundsException e) { 
			// Looks like the deviceList has no selection
			return null;
		}
	}

	/**
	 * Utility function to clear the data displayed by the device status panel.
	 */
	protected void clearDeviceStatus() {
		deviceDescription.setText("");
		deviceStatus.setText("");
		clientStatus.setText("");
		usersConnected.setText("");
		timeConnected.setText("");
		
		connectButton.setEnabled(false);
		disconnectButton.setEnabled(false);
	}

	/**
	 * Initialise all the listeners for the form.
	 */
	protected void initListeners() { 
		connectButton.addActionListener(new connectButtonListener());
		disconnectButton.addActionListener(new disconnectButtonListener());

		serverList.addItemListener(serverListListener);
		deviceList.addItemListener(deviceListListener);
		
		linkServers.addObserver(linkServersObserver);
	}
	
	/**
	 * Create all the controls on the main panel and initialise them to their defaults.
	 */
	protected void initForm() { 
		GridBagLayout gridBag = new GridBagLayout();
		GridBagConstraints gbc = new GridBagConstraints();
		
		mainPanel.setLayout(gridBag);
		
		gbc.insets = new Insets(5, 5, 5, 5);
		
		// Add the labels
		gbc.fill = GridBagConstraints.BOTH;
		gbc.weightx = 0;
		gbc.weighty = 0;
		gbc.gridwidth = 1;
		gridBag.setConstraints(serverLabel, gbc);
		mainPanel.add(serverLabel);
		gridBag.setConstraints(deviceLabel, gbc);
		mainPanel.add(deviceLabel);
		gbc.weightx = 1;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gridBag.setConstraints(statusLabel, gbc);
		mainPanel.add(statusLabel);
		
		// Next row.  Couple of lists and a panel
		gbc.weightx = 0;
		gbc.weighty = 1;
		gbc.gridwidth = 1;
		gridBag.setConstraints(serverList, gbc);
		mainPanel.add(serverList);
		gridBag.setConstraints(deviceList, gbc);
		mainPanel.add(deviceList);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gbc.weightx = 1;
		Panel statusPanel = new Panel();
		gridBag.setConstraints(statusPanel, gbc);
		initStatusPanel(statusPanel);
		mainPanel.add(statusPanel);
	}
	
	/**
	 * Initialises the status sub-panel, laying out the components and setting
	 * their properties.
	 */
	protected void initStatusPanel(Panel panel) { 
		GridBagLayout gridBag = new GridBagLayout();
		GridBagConstraints gbc = new GridBagConstraints();
		panel.setLayout(gridBag);
		
		gbc.insets = new Insets(5, 5, 5, 5);
		gbc.fill = GridBagConstraints.HORIZONTAL;
		gbc.gridwidth = 1;
		gbc.weightx = 0;
		gridBag.setConstraints(deviceDescriptionLabel, gbc);
		panel.add(deviceDescriptionLabel);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gbc.weightx = 1;
		gridBag.setConstraints(deviceDescription, gbc);
		panel.add(deviceDescription);
		deviceDescription.setEditable(false);

		gbc.gridwidth = 1;
		gbc.weightx = 0;
		gridBag.setConstraints(clientStatusLabel, gbc);
		panel.add(clientStatusLabel);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gbc.weightx = 1;
		gridBag.setConstraints(clientStatus, gbc);
		panel.add(clientStatus);
		clientStatus.setEditable(false);
		
		gbc.gridwidth = 1;
		gbc.weightx = 0;
		gridBag.setConstraints(deviceStatusLabel, gbc);
		panel.add(deviceStatusLabel);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gbc.weightx = 1;
		gridBag.setConstraints(deviceStatus, gbc);
		panel.add(deviceStatus);
		deviceStatus.setEditable(false);
		
		gbc.gridwidth = 1;
		gbc.weightx = 0;
		gridBag.setConstraints(usersConnectedLabel, gbc);
		panel.add(usersConnectedLabel);
		Panel upInfoPanel = new Panel();
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gbc.weightx = 1;
		gbc.insets = new Insets(0, 0, 0, 0);
		gridBag.setConstraints(upInfoPanel, gbc);
		panel.add(upInfoPanel);
		
		upInfoPanel.setLayout(gridBag);
		gbc.gridwidth = 1;
		gbc.weightx = 1;
		gbc.insets = new Insets(5, 5, 5, 5);
		gridBag.setConstraints(usersConnected, gbc);
		upInfoPanel.add(usersConnected);
		usersConnected.setEditable(false);
		gbc.weightx = 0;
		gridBag.setConstraints(timeConnectedLabel, gbc);
		upInfoPanel.add(timeConnectedLabel);
		gbc.weightx = 1;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gridBag.setConstraints(timeConnected, gbc);
		upInfoPanel.add(timeConnected);
		timeConnected.setEditable(false);
		
		Panel buttonPanel = new Panel();
		buttonPanel.setLayout(gridBag);
		gbc.fill = GridBagConstraints.BOTH;
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gbc.weightx = 1;
		gridBag.setConstraints(buttonPanel, gbc);
		panel.add(buttonPanel);
		
		gbc.fill = GridBagConstraints.NONE;
		gbc.gridwidth = 1;
		gridBag.setConstraints(connectButton, gbc);
		buttonPanel.add(connectButton);
		connectButton.setEnabled(false);
		gbc.gridwidth = GridBagConstraints.REMAINDER;
		gridBag.setConstraints(disconnectButton, gbc);
		buttonPanel.add(disconnectButton);
		disconnectButton.setEnabled(false);
	}
}
