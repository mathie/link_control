public class linkServerList extends java.util.Observable { 
	
	/**
	 * The internal representation of the Server list
	 */
	protected java.util.Vector serverList = new java.util.Vector();
	
	 /**
	  * Member function add, adds a server to the current list.  If it
	  * is already there, it raises a ServerExistsException.
	  */
	public synchronized void add(linkServer newServer) throws ServerExistsException { 
		if (exists (newServer))
			throw new ServerExistsException(newServer.getAddress().getHostAddress());
		
		serverList.addElement(newServer);
		setChanged();
		notifyObservers();
	}
	
	/** 
	 * Member function to remove a server from the list of servers.
	 * Throws NoSuchServerException if the server doesn't exist.
	 */
	public synchronized void remove(linkServer server) throws NoSuchServerException { 
		if (!serverList.removeElement(server))
			throw new NoSuchServerException(server.getAddress().getHostAddress());
		setChanged();
		notifyObservers();
	}
	
	/**
	 * Returns the server associated with the IP address.  If the server does not
	 * exist, it throws NoSuchServerException.
	 */
	public linkServer getServer(java.net.InetAddress addr) throws NoSuchServerException { 
		int size = serverList.size();
		for (int index = 0; index < size; index++) { 
			linkServer currentServer = (linkServer)serverList.elementAt(index);
			if (currentServer.equals(addr))
				return currentServer;
		}
		throw new NoSuchServerException(addr.getHostAddress());
	}
	
	/**
	 * return an enumeration of all the servers themselves.
	 */
	public java.util.Enumeration getServers() {
		return serverList.elements();
	}
	
	/**
	 * return a list of all the server names (or IP addresses).
	 */
	public java.util.Enumeration getServerNames() { 
		
		java.util.Vector names = new java.util.Vector();
		java.util.Enumeration enum = getServers();
		while (enum.hasMoreElements()) { 
			// TODO: What happens when there is no host name to match the number?
			names.addElement(((linkServer)enum.nextElement()).getAddress().getHostName());
		}
		return names.elements();
	}
	
	/**
	 * returns true if the server is already in the list, false otherwise.
	 */
	public boolean exists(linkServer server) { 
		int size = serverList.size();
		for (int index = 0; index < size; index++) { 
			if (server.equals((linkServer)serverList.elementAt(index)))
				return true;
		}
		return false;
	}
}
