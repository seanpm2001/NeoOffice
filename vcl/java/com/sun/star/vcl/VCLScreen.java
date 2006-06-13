/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 by Patrick Luby (patrick.luby@planamesa.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

package com.sun.star.vcl;

import java.awt.Dimension;
import java.awt.Frame;
import java.awt.Graphics2D;
import java.awt.GraphicsConfiguration;
import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.Insets;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.SystemColor;
import java.awt.Toolkit;
import java.awt.Window;
import java.awt.image.BufferedImage;

/** 
 * The Java class that implements the convenience methods for accessing
 * screen information.
 * <p>
 * @version 	$Revision$ $Date$
 * @author 	    $Author$
 */
public final class VCLScreen {

	/**
	 * The minimum screen resolution.
	 */
	public final static int MIN_SCREEN_RESOLUTION = 96;

	/**
	 * The default screen bounds.
	 */
	private static Rectangle defaultScreenBounds = new Rectangle(0, 0, 800, 600);

	/**
	 * The cached frame insets.
	 */
	private static Insets frameInsets = null;

	/**
	 * The cached minimum frame size.
	 */
	private static Dimension minimumFrameSize = null;

	/**
	 * Cached buffered image.
	 */
	private static BufferedImage image = new BufferedImage(1, 1, BufferedImage.TYPE_INT_ARGB_PRE);

	/**
	 * Initialize screen size and frame insets.
	 */
	static {

		Frame f = new Frame();
		f.setSize(1, 1);
		f.addNotify();
		frameInsets = f.getInsets();
		minimumFrameSize = f.getSize();
		f.removeNotify();
		f.dispose();

	}

	/**
	 * Returns the <code>System.control</code>.
	 *
	 * @return the <code>System.control</code>
	 */
	public static int getControlColor() {

		int color = 0x00000000;

		Graphics2D g = image.createGraphics();
		if (g != null) {
			try {
				g.setColor(SystemColor.control);
				g.fillRect(0, 0, 1, 1);
				color = image.getRGB(0, 0);
			}
			catch (Throwable t) {
				t.printStackTrace();
			}
			g.dispose();
		}

		return color;

	}

	/**
	 * Gets the <code>Frame</code> insets.
	 *
	 * @return the <code>Frame</code> insets
	 */
	public static Insets getFrameInsets() {

		return frameInsets;

	}

	/**
	 * Gets the minimum <code>Frame</code> size.
	 *
	 * @return the minimum <code>Frame</code> size
	 */
	public static Dimension getMinimumFrameSize() {

		return minimumFrameSize;

	}

	/**
	 * Returns the screen bounds that is the closest match for the specified
	 * bounds.
	 *
	 * @param x the x coordinate 
	 * @param y the y coordinate 
	 * @param width the width
	 * @param height the height
	 * @param fullScreenMode if <code>true</code> ignore screen insets
	 *  otherwise include screen insets in the screen bounds
	 * @param useMainScreen if <code>true</code> return the main screen's
	 *  bounds otherwise return the closest matching screen's bounds
	 * @return if the specified width and height are both greater than zero,
	 *  the screen bounds that is the closest match for the specified
	 *  coordinates is returned otherwise the entire virtual bounds is returned
	 */
	public static Rectangle getScreenBounds(int x, int y, int width, int height, boolean fullScreenMode, boolean useMainScreenOnly) {

		GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
		if (ge != null) {
			if (width <= 0 || height <= 0) {
				// Iterate through the screens and return the total virtual
				// screen bounds
				Rectangle virtualBounds = null;
				GraphicsDevice[] gd = ge.getScreenDevices();
				for (int i = 0; i < gd.length; i++) {
					Rectangle r = gd[i].getDefaultConfiguration().getBounds();
					if (virtualBounds != null)
						virtualBounds = virtualBounds.union(r);
					else
						virtualBounds = r;
				}

				return virtualBounds;
			}
			else if (useMainScreenOnly) {
				GraphicsDevice gd = ge.getDefaultScreenDevice();
				if (gd != null) {
					Rectangle r = gd.getDefaultConfiguration().getBounds();
					if (!fullScreenMode) {
						Insets insets = VCLScreen.getScreenInsets(gd);
						if (insets != null)
							r = new Rectangle(r.x + insets.left, r.y + insets.top, r.width - insets.left - insets.right, r.height - insets.top - insets.bottom);
					}

					if (r.isEmpty())
						return VCLScreen.defaultScreenBounds;
					else
						return r;
				}
			}
			else {
				// Iterate through the screens and find the screen that the
				// point is inside of
				GraphicsDevice[] gd = ge.getScreenDevices();
				for (int i = 0; i < gd.length; i++) {
					Rectangle r = gd[i].getDefaultConfiguration().getBounds();
					if (!fullScreenMode) {
						Insets insets = VCLScreen.getScreenInsets(gd[i]);
						if (insets != null)
							r = new Rectangle(r.x + insets.left, r.y + insets.top, r.width - insets.left - insets.right, r.height - insets.top - insets.bottom);
					}

					// Test if the point is inside the screen
					if (r.contains(x, y))
						return r;
				}

				// Iterate through the screens and find the closest screen
				long closestArea = Long.MAX_VALUE;
				Rectangle closestBounds = null;
				for (int i = 0; i < gd.length; i++) {
					Rectangle r = gd[i].getDefaultConfiguration().getBounds();
					if (!fullScreenMode) {
						Insets insets = VCLScreen.getScreenInsets(gd[i]);
						if (insets != null)
							r = new Rectangle(r.x + insets.left, r.y + insets.top, r.width - insets.left - insets.right, r.height - insets.top - insets.bottom);
					}

					// Test the closeness of the point to the center of the
					// screen
					long area = Math.abs((r.x + (r.width / 2) - x) * (r.y + (r.height / 2) - y));
					if (closestArea > area) {
						closestArea = area;
						closestBounds = r;
					}
				}

				if (closestBounds == null || closestBounds.isEmpty())
					return VCLScreen.defaultScreenBounds;
				else
					return closestBounds;
			}
		}

		return VCLScreen.defaultScreenBounds;

	}

	/**
	 * Returns the insets for the specified <code>GraphicsDevice</code>.
	 *
	 * @param gd the <code>GraphicsDevice</code>
	 * @return the insets for the specified <code>GraphicsDevice</code>
	 */
	public static native Insets getScreenInsets(GraphicsDevice gd);

	/**
	 * Returns the <code>System.textHighlight</code>.
	 *
	 * @return the <code>System.textHighlight</code>
	 */
	public static int getTextHighlightColor() {

		int color = 0x00000000;

		Graphics2D g = image.createGraphics();
		if (g != null) {
			try {
				g.setColor(SystemColor.textHighlight);
				g.fillRect(0, 0, 1, 1);
				color = image.getRGB(0, 0);
			}
			catch (Throwable t) {
				t.printStackTrace();
			}
			g.dispose();
		}

		return color;

	}

	/**
	 * Returns the <code>System.textHighlightText</code>.
	 *
	 * @return the <code>System.textHighlightText</code>
	 */
	public static int getTextHighlightTextColor() {

		int color = 0x00000000;

		Graphics2D g = image.createGraphics();
		if (g != null) {
			try {
				g.setColor(SystemColor.textHighlightText);
				g.fillRect(0, 0, 1, 1);
				color = image.getRGB(0, 0);
			}
			catch (Throwable t) {
				t.printStackTrace();
			}
			g.dispose();
		}

		return color;

	}

	/**
	 * Returns the <code>System.textText</code>.
	 *
	 * @return the <code>System.textText</code>
	 */
	public static int getTextTextColor() {

		int color = 0x00000000;

		Graphics2D g = image.createGraphics();
		if (g != null) {
			try {
				g.setColor(SystemColor.textText);
				g.fillRect(0, 0, 1, 1);
				color = image.getRGB(0, 0);
			}
			catch (Throwable t) {
				t.printStackTrace();
			}
			g.dispose();
		}

		return color;

	}

}
