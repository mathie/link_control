public class Timespan { 
	
	/**
	 * Convert a timespan (in seconds) into a string equivalent
	 * 
	 * TODO: Internationalise the Timespan class
	 */
	public static String toString(long time) { 
		long seconds = time;
		long minutes = time /  60;
		long hours   = time / (60 * 60);
		long days    = time / (60 * 60 * 24);
		
		if (days > 0)
			return new String(Long.toString(days) + " Days, " + Long.toString(hours - days * 24) + " Hours.");
		else if (hours > 0)
			return new String(Long.toString(hours) + " Hours, " + Long.toString(minutes - hours * 60) + " Minutes.");
		else if (minutes > 0)
			return new String(Long.toString(minutes) + " Minutes, " + Long.toString(seconds - minutes * 60) + " Seconds.");
		else
			return new String(Long.toString(seconds) + " Seconds.");
	}
		 
}
