import java.net.*;

public class serverCommunicator { 
	
	/**
	 * Constructor which creates a new object that talks to a particular address and
	 * port.
	 */
	public serverCommunicator(InetAddress Address, int Port) throws SocketException { 
		this.Address = Address;
		this.Port = Port;
		this.Socket = new DatagramSocket();
		this.Socket.setSoTimeout(DefaultTimeout);
	}
	
	/**
	 * Internet address of the server that we communicate with.  Set by the constructor
	 * and not alterable.
	 */
	protected InetAddress Address = null;
	
	/** 
	 * Port that the server is alledged to be listening on.  Set by the constructor and
	 * not alterable.
	 */
	protected int         Port = 0;
	
	/**
	 * The socket that this class uses to communicate with the server.  It's internal to
	 * the class and initialised by the constructor.
	 */
	protected DatagramSocket Socket = null;
	
	/**
	 * The Default timeout in milliseconds.
	 */
	protected int         DefaultTimeout = 1000; // milliseconds
	
	/**
	 * Facility for setting the default timeout.  Takes an argument in milliseconds.
	 */
	public synchronized void setTimeout(int Timeout) throws SocketException {
		Socket.setSoTimeout(Timeout);
	}

	/**
	 * The default number of retries before it fails with an exception.
	 */
	protected int		  Retries = 3;
	
	/**
	 * Facility for altering the number of retries before it fails with an exception.
	 */
	public synchronized void setRetries(int Retries) { 
		this.Retries = Retries;
	}

	/**
	 * Sends a message to the server and waits until the specified timeout for a response.
	 * It simply calls sendMessage and getMessage in turn.
	 */
	public synchronized String sendMessageWithResponse(String message) throws java.io.IOException { 
		sendMessage(message);
		return getMessage();
	}
	
	/**
	 * Sends a message to the server.  Pretty much, it shouldn't fail.  If anything, it will
	 * just silently lose the message...  But, hey, that's a _feature_ of UDP.
	 */
	public synchronized void sendMessage(String messageStr) throws java.io.IOException {
		byte messageBytes[] = null;
		try { 
			messageBytes = messageStr.getBytes("UTF8");
		} catch (java.io.UnsupportedEncodingException e) { 
			throw new Error(e.getMessage());
		}
		
		// Create the packet of data and send it
		DatagramPacket packet = new DatagramPacket(messageBytes, messageBytes.length, Address, Port);
		Socket.send(packet);
	}
	
	/**
	 * Waits for a message from the server.  If nothing turns up within a specified timeout,
	 * raise an exception.  THe algorithm for the timeout is a simple exponential backoff
	 * type thing.  It first tries with the specified timeout.  If that fails, then it
	 * doubles the timeout and decrements retries.  If retries hits zero, it raises the
	 * exception.
	 */
	public synchronized String getMessage() throws java.io.InterruptedIOException {
		DatagramPacket packet = new DatagramPacket(new byte[protocol.maxPacketSize],
												   protocol.maxPacketSize);
		
		int timeout = DefaultTimeout;
		try {
			timeout = Socket.getSoTimeout();
		} catch (SocketException e) { 
			// The default will have to do here.
		}
		int retries = this.Retries;
		while (retries > 0) { 
			try { 
				Socket.receive(packet);
				return new String(packet.getData(), "UTF8").trim();
			} catch (java.io.UnsupportedEncodingException e) {
				  throw new Error(e.getMessage());
			} catch (java.io.IOException e) { 
				// Failed to receive information.  If we've got retries left, increase
				// the timeout and try again.
				retries--;
				try {
					Socket.setSoTimeout(Socket.getSoTimeout() * 2);
				} catch (SocketException e2) { 
					// stay with the existing timeout, I suppose
				}
			} finally { 
				// Restore the initial timeout
				try {
					Socket.setSoTimeout(timeout);
				} catch (SocketException e2) { 
					// stay with the existing timeout, I suppose
				}
			}
		}
		throw new java.io.InterruptedIOException();
	}
}
