package cz.mosra.magnum.JavaViewer;

import java.awt.Canvas;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import javax.swing.Timer;
import javax.swing.JFrame;
import javax.swing.JButton;

public class NativeCanvas extends Canvas implements ActionListener, ComponentListener, MouseListener, MouseMotionListener, MouseWheelListener {
    static final long serialVersionUID = 0L;

    static {
        System.loadLibrary("NativeCanvas");
    }

    public final static int PERIOD = 16;
    private boolean constructed = false;
    private boolean redraw = true;

    NativeCanvas() {
        addComponentListener(this);
        addMouseListener(this);
        addMouseMotionListener(this);
        addMouseWheelListener(this);
    }

    public void setRedraw() {
        redraw = true;
    }

    public void actionPerformed(ActionEvent evt) {
        if (!constructed) {
            construct();
            constructed = true;
        }

        if(redraw) {
            draw();
            redraw = false;
        }
    }

    public void componentResized(ComponentEvent e) {
        if(getWidth() == 0 || getHeight() == 0)
            return;

        if (!constructed) {
            construct();
            constructed = true;
        }

        redraw = true;
        setViewport(getWidth(), getHeight());
    }

    public void mousePressed(MouseEvent e) {
        press(e.getX(), e.getY());
    }

    public void mouseDragged(MouseEvent e) {
        redraw = true;
        drag(e.getX(), e.getY());
    }

    public void mouseReleased(MouseEvent e) {
        release();
    }

    public void mouseWheelMoved(MouseWheelEvent e) {
        redraw = true;
        zoom(e.getWheelRotation());
    }

    public void addNotify() {
        super.addNotify();
        Timer timer = new Timer(PERIOD, this);
        timer.start();
    }

    public void removeNotify() {
        super.removeNotify();
        destruct();
    }

    public native void construct();
    public native void destruct();
    public native void setViewport(int width, int height);
    public native void press(int x, int y);
    public native void release();
    public native void drag(int x, int y);
    public native void zoom(int direction);
    public native void draw();

    /* Unused stuff. */
    public void componentHidden(ComponentEvent e) {}
    public void componentMoved(ComponentEvent e) {}
    public void componentShown(ComponentEvent e) {}
    public void mouseClicked(MouseEvent e) {}
    public void mouseEntered(MouseEvent e) {}
    public void mouseExited(MouseEvent e) {}
    public void mouseMoved(MouseEvent e) {}
}
