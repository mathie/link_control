/**
 * linkDeviceList is intended to store a list of devices connected
 * to a server.  It should generally only be a small list, so the
 * implementation has not been designed with efficiency in mind, only
 * for ease of coding. :)
 */
public class linkDeviceList extends java.util.Observable { 
	
	/**
	 * The internal representation of the device list
	 */
	protected java.util.Vector deviceList = new java.util.Vector();
	
	 /**
	  * Member function add, adds a device to the current list.  If it
	  * is already there, it raises a DeviceExistsException.
	  */
	public synchronized void add(linkDevice newDevice) throws DeviceExistsException { 
		if (exists (newDevice))
			throw new DeviceExistsException(newDevice.getName());
		
		deviceList.addElement(newDevice);
		setChanged();
	}
	
	/** 
	 * Member function to remove a device from the list of devices attached
	 * to a server.  Throws NoSuchDeviceException if the device doesn't
	 * exist.
	 */
	public synchronized void remove(linkDevice device) throws NoSuchDeviceException { 
		if (!deviceList.removeElement(device))
			throw new NoSuchDeviceException(device.getName());
		setChanged();
	}
	
	/**
	 * Returns the device associated with the name.  If the device does not
	 * exist, it throws NoSuchDeviceException.
	 */
	public linkDevice getDevice(String name) throws NoSuchDeviceException { 
		int size = deviceList.size();
		for (int index = 0; index < size; index++) { 
			linkDevice currentDevice = (linkDevice)deviceList.elementAt(index);
			if (currentDevice.equals(name))
				return currentDevice;
		}
		throw new NoSuchDeviceException(name);
	}
	
	/**
	 * return a list of all the device names.
	 */
	public java.util.Enumeration getDeviceNames() { 
		
		java.util.Vector names = new java.util.Vector();
		java.util.Enumeration enum = deviceList.elements();
		while (enum.hasMoreElements()) { 
			names.addElement(((linkDevice)enum.nextElement()).getName());
		}
		return names.elements();
	}
	

	/**
	 * returns true if the device is already in the list, false otherwise.
	 */
	public boolean exists(linkDevice device) { 
		int size = deviceList.size();
		for (int index = 0; index < size; index++) { 
			if (device.equals((linkDevice)deviceList.elementAt(index)))
				return true;
		}
		return false;
	}
	
	
}
