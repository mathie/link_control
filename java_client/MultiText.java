// Multiple line text display (not editing) component

import java.awt.*;
import java.util.*;

public class MultiText extends Canvas
{
	protected String text;
	protected int align = TOP_LEFT;
	protected Frame parent = null;
	
	// alignment variables
	public static final int TOP_LEFT = 0;
	public static final int TOP_CENTER = 1;
	public static final int TOP_RIGHT = 2;
	public static final int MIDDLE_LEFT = 3;
	public static final int MIDDLE_CENTER = 4;
	public static final int MIDDLE_RIGHT = 5;
	public static final int BOTTOM_LEFT = 6;
	public static final int BOTTOM_CENTER = 7;
	public static final int BOTTOM_RIGHT = 8;

	public MultiText(String t) { 
		this(null, t);
	}
	
	public MultiText(Frame parent, String t)
	{
		this(parent, t, TOP_LEFT);
	}
	
	public MultiText(String text, int align) { 
		this(null, text, align);
	}
	
	public MultiText(Frame parent, String text, int align)
	{
		super();
		this.text = text;
		this.align = align;
		this.parent = parent;
	}
	
	public void paint(Graphics g)
	{
		IndexPair curInd;
		
		g.setFont(getFont()); // TODO: do I need this?
		FontMetrics fM = g.getFontMetrics();
		int x, curHeight, oneHeight = fM.getHeight();
		int maxWidth = getBounds().width;
		possBreakUp(maxWidth);
		Enumeration lBs = breaks.elements();
		// Find top of text
		switch (align / 3) {
		case 1: curHeight=(getBounds().height-height)/2; break;
		case 2: curHeight= getBounds().height-height;    break;
		default: curHeight=0;
		}
		
		while (lBs.hasMoreElements()) {
			curHeight += oneHeight;
			curInd = (IndexPair) lBs.nextElement();
			// Find starting x coord
			switch (align % 3) {
			case 1: x=(maxWidth-curInd.width)/2;	break;
			case 2: x= maxWidth-curInd.width;		break;
			default: x=0;							break;
			}
			g.drawString(text.substring(curInd.start, curInd.end), x, curHeight);
		}
	}
	
	public void setAlignment(int alignment)
	{
		align = alignment;
	}
	
	public int getAlignment()
	{
		return align;
	}
	
	public Dimension getPreferredSize() { 
		
		// By preference, we base our size on the size of the parent frame
		if (parent != null) { 
			possBreakUp((parent.getSize().width * 4) / 5);
		} else {
			// Wrap with max length in pixels (taken to be approx 80% of screen)
			possBreakUp((getToolkit().getScreenSize().width*4)/5);
		}
		// That has just screwed up the stored stuff, so mark it bad (should really calculate these independantly)
		breaks = null;
		return new Dimension(width, height);
	}
	
	public Dimension getMinimumSize()
	{
		return getPreferredSize();
	}
	
	public void setBounds(int x, int y, int w, int h)
	{
		breaks=null;
		super.setBounds(x, y, w, h);
	}
	
	public void setBounds(Rectangle r)
	{
		breaks=null;
		super.setBounds(r);
	}
	
	public void setSize(int w, int h)
	{
		breaks=null;
		super.setSize(w, h);
	}
	
	public void setSize(Dimension d)
	{
		breaks=null;
		super.setSize(d);
	}

	/* - Deprecated
	
	public void resize(Dimension d)
	{
		breaks=null;
		super.resize(d);
	}
	
	public void resize(int w, int h)
	{
		breaks=null;
		super.resize(w, h);
	}
	
	public void reshape(int x, int y, int w, int h)
	{
		breaks=null;
		super.reshape(x, y, w, h);
	}
	*/
	
	public void setText(String t)
	{
		text = t;
		// Remove old line breaks
		breaks = null;
		repaint();
	}
	
	protected int width, height;
	protected Vector breaks = null;

	protected class IndexPair {
		public int start, end, width;
		public IndexPair(int s, int e, int w) { start=s; end=e; width=w; }
	}
	
	// Recalculates line breaks if something has changed or breaks have not otherwise been calculated
	public void possBreakUp(int maxWidth)
	{
		if (breaks == null)
			breakUpText(maxWidth);
	}
	
	public void setFont(Font f)
	{
		super.setFont(f);
		// Breaks may need recalculating
		breaks = null;
	}
	
	protected void breakUpText(int maxWidth)
	{
		int curIndex = 0, bestIndex, newIndex;
		int curWidth, newWidth;
		int txtLen = text.length();
		int lines = 0;
		breaks = new Vector();
		FontMetrics fM = getFontMetrics(getFont());
		width = 0;
		
		while (curIndex < txtLen) {
			// Add words until line too long, then take previous value
			newIndex = bestIndex = curIndex+1; // TODO: fix case where word is too large for line properly
			curWidth = 0;
			while( (newWidth = fM.stringWidth(text.substring(curIndex, newIndex))) < maxWidth
				  && bestIndex < txtLen) {
				curWidth = newWidth;
				bestIndex = newIndex;
				newIndex = text.indexOf(' ', bestIndex+1);
				if (newIndex == -1)
					newIndex = txtLen;
			}
			breaks.addElement(new IndexPair(curIndex, bestIndex, curWidth));
			if (width < curWidth) width = curWidth;
			curIndex = bestIndex+1;
			lines++;
			}
		height = (lines + 1) * fM.getHeight();
	}

	public String getText()
	{
		return text;
	}
}
