import java.net.*;
import java.util.StringTokenizer;

public class multicastThread extends Thread {
	/**
	 * Keep a copy of the multicastSocket we listen on.  The object is created in the
	 * linkControl constructor.
	 */
	protected MulticastSocket multicastSocket = null;
	
	/**
	 * Keep a reference to the master list of servers.  This list is created by the
	 * linkControl constructor.
	 */
	protected linkServerList linkServers = null;
	
	/**
	 * The only public constructor requires a reference to the multicast socket and
	 * a list of servers.
	 */
	public multicastThread (MulticastSocket multicastSocket, linkServerList linkServers) {
		this.multicastSocket = multicastSocket;
		this.linkServers = linkServers;
	}
	
	/**
	 * The purpose of this thread (and hence its run() method) is to listen
	 * for new datagrams on the multicast socket and deal with them appropriately.
	 * This will most likely involve updating the server list.
	 */
	public void run() { 
		
		while (true /* should I have a better end case? */) {
			DatagramPacket packet = new DatagramPacket(new byte[protocol.maxPacketSize],
													   protocol.maxPacketSize);
			
			try {
				// First ensure that the timeout it infinite - it doesn't really matter
				// how long this thread waits for, it's got nothing better to do.
				multicastSocket.setSoTimeout(0 /* INFINITE */);
				multicastSocket.receive(packet);
			} catch (java.io.IOException e) {
				// TODO: handle this error.  Warn the user and say that if it happens
				// again, they should quit the program and investigate.
				System.err.println("Failed to receive packet from multicast socket: " + e.getLocalizedMessage());
				continue;
			}
			
			try {
				// First of all, see about the server
				linkServer currentServer = getServer(packet.getAddress(), packet.getPort());
			
				// Parse the message and deal with it
				parseMessage(packet.getData(), currentServer);
			} catch (NoSuchServerException e) {
				// Server sent us a status message then claimed not to exist. Freaky.  Just
				// ignore the status message for now.
				continue;
			}
		}
	}
	
	/**
	 * Based on the InetAddress, look for the server which has just sent it's status message
	 * to us.  If it exists, return it.  If not, create a new server, add it to the list
	 * of servers and return that.
	 */
	protected linkServer getServer (InetAddress Address, int Port) throws NoSuchServerException { 
		linkServer currentServer = null;
		try {
			currentServer = linkServers.getServer(Address);
			currentServer.updateLastHeardFrom();
			return currentServer;
		} catch (NoSuchServerException e) {
			// The server is not known to us.  Add it to the server list
			try {
				currentServer = new linkServer(Address, Port);
				linkServers.add(currentServer);
				return currentServer;
			} catch (ServerExistsException e2) { 
				// This should _NEVER_ happen.  _EVER_!  First a server claims not
				// to exist, then it does.  There will be no other thread which adds
				// servers while this one is running...
				System.err.println("This cannot happen.  Break here and read the comment!");
				throw new Error(e2.getMessage());
			}
		}
	}
	
	/**
	 * Parse the received status message.  Act on the message in the following ways:
	 * 
	 * INIT:	Remove knwoledge of all the devices and ask for a new device list
	 * STATUS:	Update the each of the devices' status
	 * QUIT:	Remove the server from our list.
	 */
	protected void parseMessage (byte messageBytes[], linkServer server) { 
		StringTokenizer toke = null;
		try { 
			toke = new StringTokenizer(new String(messageBytes, "UTF8").trim(), " \n\t");
		} catch (java.io.UnsupportedEncodingException e) { 
			// If this happens, We're fu'd anyway.
			throw new Error(e.getLocalizedMessage());
		}
		
		// The first token should be the source of the message.  This function
		// should only receive messages from the broadcast source.
		if (!toke.nextToken().equals(protocol.broadcastPrefix)) { 
			// TODO: warn the user of an invalid message received
			System.err.println("Warning:  received an invalid message.");
			throw new Error();
		}
		
		// Find out what the actual message was
		String message = toke.nextToken();
		if (message.compareTo(protocol.broadcastInit) == 0) { 
			try {
				server.reInit();
			} catch (NoSuchServerException e) { 
				// The server didn't respond to a request for device information.
				// Remove it from our list of servers
				try {
					linkServers.remove(server);
				} catch (NoSuchServerException e2) { 
					// If the server has already been removed, then it's not a problem.
				}
			}
		} else if (message.equals(protocol.broadcastQuit)) {
			try { 
				linkServers.remove(server);
			} catch (NoSuchServerException e) { 
				// If the server doesn't exist anyway, now, then our job is done...
			}
		} else if (message.equals(protocol.broadcastStatus)) {
			server.updateDeviceStatus(toke);
		} else {
			// TODO: warn the user of an invalid message received
			System.err.println("Warning:  received an invalid message.");
			throw new Error();
		}
	}
}
