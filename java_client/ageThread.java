/**
 * The ageThread class regularly watches the linkServers data structure
 * to see when a server last communicated with a client.  If a server has
 * not been heard from in a while, it is removed from the client.  Conversely,
 * it is also aware that if a client does not talk to the server regularly,
 * the client will be dropped from the server's data structures, so it
 * occasionally polls the server anyway.
 */
public class ageThread extends Thread {
	/**
	 * Only constructor takes a reference to the linkServers list
	 */
	public ageThread(linkServerList linkServers) { 
		this.linkServers = linkServers;
	}
	
	/**
	 * The following defaults apply to the server:
	 * 
	 * multicast broadcast:		every 5 seconds
	 * client timeout:			2 hours
	 */
	protected long serverPollFrequency = 5000; // milliseconds
	protected long serverForgetClientFrequency = 7200000; // milliseconds (2 hours)
	
	/**
	 * So we'll take the following defaults for the client:
	 * 
	 * The client must poll the server 4 times before it times out.  That way, several
	 * messages would have to be missed.
	 * 
	 * The client should timeout the server if it misses 4 consecutive messages.
	 */
	protected long clientPollFrequency = serverForgetClientFrequency / 4;
	protected long clientForgetServerFrequency = serverPollFrequency * 4;
	
	/**
	 * How long we should sleep in between checking timeouts.  I reckon we should check
	 * at least twice as often as the smallest timeout.  Just so they don't age too much.
	 */
	protected long sleepTime = clientForgetServerFrequency / 2;
	
	/**
	 * Keep ourselves a reference to the linkServers list.
	 */
	protected linkServerList linkServers;
	
	/**
	 * The main procedure of this thread.  It should run infinitely while the rest of the
	 * program is running.
	 */
	public void run() { 
		while (true /* The energizer while loop. :) */) { 
			java.util.Enumeration servers = linkServers.getServers();
			while (servers.hasMoreElements()) { 
				linkServer currentServer = (linkServer)servers.nextElement();
				
				// First check to see if the server is past its communicate-by date.
				long timeSince = System.currentTimeMillis() - currentServer.getLastHeardFrom();
				if (timeSince > clientForgetServerFrequency) { 
					try {
						linkServers.remove(currentServer);
					} catch (NoSuchServerException e) { 
						// Sombody beat us to it.  Never mind.
					}
					linkServers.notifyObservers();
				}
				
				// See if the server needs to be polled before it's removed
				timeSince = System.currentTimeMillis() - currentServer.getLastTalkedTo();
				if (timeSince > clientPollFrequency) { 
					try {
						serverCommunicator talk = new serverCommunicator(currentServer.getAddress(),
																		 currentServer.getPort());
						talk.sendMessage(protocol.clientPrefix + " " + protocol.clientPing);
						currentServer.updateLastTalkedTo();
					} catch (java.io.IOException e) {
						// The server's not listening?  What do I think about that?
						throw new Error();
					}
				}
			}
			// Finally, after all that hard work, sleep it off
			try {
				sleep(sleepTime);
			} catch (InterruptedException e) { 
				// I guess somebody has a good reason for interrupting us.  Carry on.
			}
		}
	}
}
