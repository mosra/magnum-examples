package cz.mosra.magnum.JavaViewer;

import javax.swing.JFrame;
import javax.swing.JButton;
import javax.swing.UIManager;
import java.awt.BorderLayout;
import java.awt.Canvas;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import javax.swing.Timer;

public class JavaViewer extends JFrame {
    public JavaViewer() {
        super("Magnum/Java Viewer");
        setSize(800, 600);
        add(BorderLayout.CENTER, new NativeCanvas());
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }

    public static void main(String[] args) {
        try {
            String laf = UIManager.getSystemLookAndFeelClassName();

            /* Try to make it look not so terribly awful on Unices */
            if(laf == "javax.swing.plaf.metal.MetalLookAndFeel")
                UIManager.setLookAndFeel("com.sun.java.swing.plaf.gtk.GTKLookAndFeel");

            /* It works for Windows (only?!) */
            else UIManager.setLookAndFeel(laf);

        /* Fallback to terrible awfulness */
        } catch(Exception e) {}

        JavaViewer jv = new JavaViewer();
        jv.setVisible(true);
    }
}
