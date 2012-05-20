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
    private int redraw = 0;

    NativeCanvas() {
        addComponentListener(this);
        addMouseListener(this);
        addMouseMotionListener(this);
        addMouseWheelListener(this);
    }

    public void setRedraw() {
        redraw = 2; /* Twice to fix issues when resizing window */
    }

    @Override
    public void actionPerformed(ActionEvent evt) {
        if (!constructed) {
            construct();
            constructed = true;
        }

        if(redraw != 0) {
            draw();
            --redraw;
        }
    }

    @Override
    public void componentResized(ComponentEvent e) {
        if(getWidth() == 0 || getHeight() == 0)
            return;

        if (!constructed) {
            construct();
            constructed = true;
        }

        setRedraw();
        setViewport(getWidth(), getHeight());
    }

    @Override
    public void mousePressed(MouseEvent e) {
        press(e.getX(), e.getY());
    }

    @Override
    public void mouseDragged(MouseEvent e) {
        setRedraw();
        drag(e.getX(), e.getY());
    }

    @Override
    public void mouseReleased(MouseEvent e) {
        release();
    }

    @Override
    public void mouseWheelMoved(MouseWheelEvent e) {
        setRedraw();
        zoom(e.getWheelRotation());
    }

    @Override
    public void addNotify() {
        super.addNotify();
        Timer timer = new Timer(PERIOD, this);
        timer.start();
    }

    @Override
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
    @Override public void componentHidden(ComponentEvent e) {}
    @Override public void componentMoved(ComponentEvent e) {}
    @Override public void componentShown(ComponentEvent e) {}
    @Override public void mouseClicked(MouseEvent e) {}
    @Override public void mouseEntered(MouseEvent e) {}
    @Override public void mouseExited(MouseEvent e) {}
    @Override public void mouseMoved(MouseEvent e) {}
}
