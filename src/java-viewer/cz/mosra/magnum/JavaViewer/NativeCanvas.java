package cz.mosra.magnum.JavaViewer;

import javax.swing.JFrame;
import javax.swing.JButton;
import java.awt.BorderLayout;
import java.awt.Canvas;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.Timer;

public class NativeCanvas extends Canvas implements ActionListener {
    static {
        System.loadLibrary("NativeCanvas");
    }

    public final static int PERIOD = 100;

    private boolean constructed = false;

    public void actionPerformed(ActionEvent evt) {
        if (!constructed) {
            construct();
            constructed = true;
        }

        draw();
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
    public native void draw();
}
