public class linkDevice extends java.util.Observable { 
	
	/**
	 * Constructor.  Takes all the basic information which defines
	 * and device, and is constant once the device is setup
	 */
	public linkDevice(String newName, String newDescription, linkServer server) throws NoSuchServerException { 
		Name = newName;
		Description = newDescription;
		
		DeviceStatus = linkDeviceStates.UNKNOWN;
		
		Server = server;
		
		refreshClientStatus();
		
		ConnectTime = 0;
		NumUsers = 0;
	}
	
	/**
	 * Compare the device to it's parameter.  If the names of the devices
	 * match, they are considered to be equivalent and it returns true, else false.
	 */
	public boolean equals(linkDevice rhs) { 
		if (Name.equalsIgnoreCase(rhs.Name))
			return true;
		else
			return false;
	}
	
	public boolean equals(String Name) { 
		if  (Name.equalsIgnoreCase(this.Name))
			return true;
		else
			return false;
	}
	
	/**
	 * Member variable Server is a reference to the server that controls
	 * the device.  Initialised by the constructor and used only internally.
	 */
	protected linkServer Server = null;
	
	
	/**
	 * Member variable Name is the short name of the device.  Specified
	 * by the server, and used to identify the device in conversations
	 * with the server.
	 */
	protected String Name = null;
	
	/**
	 * Name access functions
	 */
	public String getName() { 
		return Name;
	}
	
	/**
	 * Member variable Description.  A longer description of the device,
	 * supplied by the server.  Purely used to describe the device to the
	 * end user.
	 */
	protected String Description = null;
	
	/**
	 * Description access functions
	 */
	public String getDescription() { 
		return Description;
	}
	
	/**
	 * Member variable DeviceStatus describes the current status of this device.
	 */
	protected int DeviceStatus = linkDeviceStates.UNKNOWN;
	
	/** 
	 * Status access functions
	 */
	public int getDeviceStatus() { 
		return DeviceStatus;
	}

	public synchronized void setDeviceStatus(int newStatus) throws InvalidStatusException { 
		if (linkDeviceStates.valid(newStatus))
			DeviceStatus = newStatus;
		else
			throw new InvalidStatusException(Integer.toString(newStatus));
		setChanged();
	}
	
	/**
	 * Member variable ClientStatus keeps track of the current status of the client
	 * in relation to the device.
	 */
	protected int ClientStatus = linkClientStates.UNKNOWN;
	
	/**
	 * ClientStatus access functions
	 */
	public int getClientStatus() {
		return ClientStatus;
	}

	/**
	 * Get the current client status from the server.  Partly used as a way to initialise
	 * the ClientStatus field, partly used as feedback to ensure that the server has recognised
	 * the last request.
	 */
	protected synchronized void refreshClientStatus() throws NoSuchServerException { 
		try {
			Server.updateLastTalkedTo();
			serverCommunicator talk = new serverCommunicator(Server.getAddress(), Server.getPort());
			String connectedDevices = talk.sendMessageWithResponse(protocol.clientPrefix + " " + protocol.clientClientStatus);
			java.util.StringTokenizer toke = new java.util.StringTokenizer(connectedDevices, " \t");
			
			// Ensure that the received message is valid.
			if (toke.nextToken().compareTo(protocol.serverPrefix) != 0) {
				ClientStatus = linkClientStates.UNKNOWN;
				return;
			}
			if (toke.nextToken().compareTo(protocol.serverClientStatus) != 0) {
				ClientStatus = linkClientStates.UNKNOWN;
				return;
			}
			
			// Parse the list of devices that this client is connected to.
			while (toke.hasMoreTokens()) { 
				String deviceName = toke.nextToken();
				if (deviceName.compareTo(getName()) == 0) {
					ClientStatus = linkClientStates.CONNECTED;
					return;
				}
			}
			
			// If it's not in the list of connected devices it must not be connected.
			ClientStatus = linkClientStates.DISCONNECTED;
			return;
		} catch (java.io.IOException e) { 
			// Anything fails, we just claim we don't know the status and throw a
			// NoSuchServerException to indicate that the server didn't respond.
			ClientStatus = linkClientStates.UNKNOWN;
			throw new NoSuchServerException(e.getLocalizedMessage());
		} finally {
			setChanged();
		}
	}
	
	/**
	 * Member variable ConnectTime is the length of time (in seconds) that
	 * the device has been connected.
	 */
	protected long ConnectTime = 0;
	
	/** 
	 * ConnectTime access functions
	 */
	public long getConnectTime() { 
		return ConnectTime;
	}
	
	public synchronized void setConnectTime(long newConnectTime) {
		ConnectTime = newConnectTime;
		setChanged();
	}

	/**
	 * Member variable NumUsers keeps a count of the number of users who are connected
	 * to a device
	 */
	protected long NumUsers = 0;
	
	/**
	 * NumUsers access functions
	 */
	public long getNumUsers() {
		return NumUsers;
	}
	
	public synchronized void setNumUsers(long newNumUsers) {
		NumUsers = newNumUsers;
		setChanged();
	}
	
	/** 
	 * Connect sends the connect command to the server, then updates the client
	 * status to reflect the fact that it has connected.
	 */
	public synchronized void Connect() throws NoSuchServerException { 
		try {
			Server.updateLastTalkedTo();
			serverCommunicator talk = new serverCommunicator(Server.getAddress(), Server.getPort());
			talk.sendMessage(protocol.clientPrefix + " " + protocol.clientUp + " " + getName());
		} catch (java.io.IOException e) { 
			// TODO: Warn the user that the attempt to connect failed.
			System.err.println("Error issuing connect command: " + e.getLocalizedMessage());
		}
		refreshClientStatus();
	}
	
	/** 
	 * Disconnect sends the disconnect command to the server, then updates the client
	 * status to reflect the fact that it has disconnected.
	 */
	public synchronized void Disconnect() throws NoSuchServerException { 
		try {
			Server.updateLastTalkedTo();
			serverCommunicator talk = new serverCommunicator(Server.getAddress(), Server.getPort());
			talk.sendMessage(protocol.clientPrefix + " " + protocol.clientDown + " " + getName());
		} catch (java.io.IOException e) { 
			// TODO: Warn the user that the attempt to connect failed.
			System.err.println("Error issuing connect command: " + e.getLocalizedMessage());
		}
		
		refreshClientStatus();
	}
}
