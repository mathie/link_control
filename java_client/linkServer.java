import java.net.InetAddress;
import java.util.Date;
import java.util.StringTokenizer;

class linkServer { 
	
	public linkServer(InetAddress serverAddress, int serverPort) throws NoSuchServerException { 
		Address = serverAddress;
		Port = serverPort;
		LastHeardFrom = System.currentTimeMillis(); // retrieve the current date
		
		DevicesControlled = null; // initialised by requestDeviceList()
		requestDeviceList();
	}
	
	/**
	 * reInit() removes knowledge about existing devices and makes a fresh request
	 * for device information.
	 */
	public synchronized void reInit() throws NoSuchServerException { 
		DevicesControlled = null; // initialised by requestDeviceList()
		requestDeviceList();
	}
	
	/**
	 * Test to see if two link servers are equivalent.  They are equivalent if the source
	 * address is equivalent.
	 */
	public boolean equals (linkServer server) { 
		if (server.getAddress().equals(Address))
			return true;
		else
			return false;
	}
	
	public boolean equals (InetAddress Address) { 
		if (Address.equals(this.Address))
			return true;
		else
			return false;
	}
	
	/**
	 * The member variable Address describes the IP address
	 * that the server is communicating on.
	 */
	protected InetAddress Address = null;
			  
	
	/**
	 * Access functions for getting the IP Address.
	 */
	public InetAddress getAddress() { 
		return Address;
	}
	
	/**
	 * The member variable Port holds the port number that the
	 * server is listening on.
	 */
	protected int Port = 0;
	
	/**
	 * Access functions for getting the server port.
	 */
	public int getPort() {
		return Port;
	}
	
	/**
	 * The member variable lastHeardFrom indicates the last time the
	 * server broadcast information to the client.  Used to age servers
	 * which seem to have died.
	 */
	protected long LastHeardFrom = 0;
	
	/**
	 * Access functions for getting and setting these values
	 */
	public long getLastHeardFrom() { 
		return LastHeardFrom;
	}
	
	public synchronized void updateLastHeardFrom() { 
		LastHeardFrom = System.currentTimeMillis();
	}
	
	/**
	 * The member variable lastTalkedTo indicates the last time we talked to 
	 * the server at all.  If we don't tell the server we do exist on a (fairly)
	 * regular basis, it'll kick us off and claim we don't exist.
	 */
	protected long LastTalkedTo = 0;
	
	/**
	 * Access functions for getting and setting the lastTalkedTo time
	 */
	public long getLastTalkedTo() {
		return LastTalkedTo;
	}
	
	public synchronized void updateLastTalkedTo() {
		LastTalkedTo = System.currentTimeMillis();
	}
	
	/**
	 * The member variable DevicesControlled has a list of all the devices
	 * that this server has control over.
	 */
	protected linkDeviceList DevicesControlled = null;
	
	/** 
	 * Access functions for the device list
	 */
	public linkDeviceList getDevicesControlled() { 
		return DevicesControlled;
	}
	
	/**
	 * Requests a list of devices pertaining to this server from the server itself.
	 * It then creates entries for each of the devices with the name & desciption
	 * supplied and unknown states.
	 */
	protected synchronized void requestDeviceList() throws NoSuchServerException { 

		// First create a new device list
		DevicesControlled = new linkDeviceList();
		
		try { 
			updateLastTalkedTo();
			serverCommunicator talk = new serverCommunicator(getAddress(), getPort());
			String deviceInformation = talk.sendMessageWithResponse(protocol.clientPrefix + " " + protocol.clientDevices);
			StringTokenizer toke = new StringTokenizer(deviceInformation, " \n\t");
			
			// Check that this message was for us
			if (toke.nextToken().compareTo(protocol.serverPrefix) != 0) { 
				System.err.println("While requesting device list, received unrecognized message.");
				throw new NoSuchServerException("Unrecognised message in requestDeviceList");
			}
			if (toke.nextToken().compareTo(protocol.serverDevices) != 0) {
				System.err.println("While requesting device list, received unrecognized message.");
				throw new NoSuchServerException("Unrecognised message in requestDeviceList");
			}
				
			while (toke.hasMoreTokens()) { 
				// Cycle through the list, getting a name and a description.  Create
				// a new device and add it to the device list
				String newName = toke.nextToken("\n\t");
				String newDescription = toke.nextToken();
				linkDevice newDevice;
				// It's possible that the device already exists in the list.  Just in case
				try {
					newDevice = DevicesControlled.getDevice(newName);
				} catch (NoSuchDeviceException e) { 
					newDevice = new linkDevice(newName, newDescription, this);
					try {
						DevicesControlled.add(newDevice);
					} catch (DeviceExistsException e2) { 
						// This should never happen
						throw new Error(e.getMessage());
					}
				} finally {
					// Tell the user interface to refresh
					DevicesControlled.notifyObservers();
				}
				// The rest of the details about the device will be updated in the next
				// status broadcast.  They are initialised to UNKNOWN by default.
			}
		} catch (java.io.IOException e) { 
			// TODO: warn the user that a request for a device list failed
			System.err.println("Failed to receive device list");
			throw new NoSuchServerException(e.getLocalizedMessage());			
		}
	}
	
	/**
	 * Update the status of all the devices listed in the status string. Also notifies the
	 * user interface for all the devices it modifies.
	 */
	public synchronized void updateDeviceStatus(StringTokenizer toke) { 
		// We have a tokenizer which should alternate between
		// a device and its status
		
		while (toke.hasMoreTokens()) { 
			String deviceName = toke.nextToken();
			String deviceStateStr = toke.nextToken();
			int deviceState = linkDeviceStates.getValue(deviceStateStr);
			if (deviceState == linkDeviceStates.INVALID) { 
				// TODO: warn the user if an invalid state turns up
				System.err.println("Warning: Invalid state received for device: " + deviceName);
				continue;
			}
			
			// Get the details of the device we're talking about
			linkDevice deviceInfo = null;
			try { 
				deviceInfo = DevicesControlled.getDevice(deviceName);
			} catch (NoSuchDeviceException e) { 
				// The DevicesControlled should be setup when the server is first instantiated
				System.err.println("This shouldn't happen.  Received status information about unknown device.");
				throw new Error(e.getMessage());
			}
			
			int connectTime = 0;
			int numUsers = 0;
			switch (deviceState) { 
			case linkDeviceStates.UP:
				// Get the next two tokens and add them too
				connectTime = Integer.parseInt(toke.nextToken());
				numUsers = Integer.parseInt(toke.nextToken());
			case linkDeviceStates.DOWN:
			case linkDeviceStates.DISCONNECTING:
			case linkDeviceStates.CONNECTING:
				try {
					deviceInfo.setConnectTime(connectTime);
					deviceInfo.setNumUsers(numUsers);
					deviceInfo.setDeviceStatus(deviceState);
				} catch (InvalidStatusException e) {
					// This shouldn't happen
					System.err.println("This shouldn't happen. I checked for it above.");
					throw new Error (e.getMessage());
				} finally {
					// Let the user interface know that the device has been updated
					deviceInfo.notifyObservers();
				}

				break;
			default:
				break;
			}
		}
	}
}
