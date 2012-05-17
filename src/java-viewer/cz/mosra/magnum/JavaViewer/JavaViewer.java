package cz.mosra.magnum.JavaViewer;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.UIManager;

public class JavaViewer extends JFrame implements ActionListener {
    static final long serialVersionUID = 0L;

    private JMenuItem openColladaAction;
    private JMenuItem closeAction;
    private NativeCanvas canvas = new NativeCanvas();

    public JavaViewer() {
        super("Magnum/Java Viewer");
        setSize(800, 600);
        add(BorderLayout.CENTER, canvas);

        /* Menu */
        JMenuBar menubar = new JMenuBar();
        setJMenuBar(menubar);

        JMenu file = new JMenu("File");
        menubar.add(file);

        openColladaAction = new JMenuItem("Open COLLADA file...");
        openColladaAction.addActionListener(this);
        file.add(openColladaAction);

        closeAction = new JMenuItem("Close file");
        closeAction.addActionListener(this);
        file.add(closeAction);

        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }

    public void actionPerformed(ActionEvent e) {
        if(e.getSource() == openColladaAction) {
            JFileChooser f = new JFileChooser();
            if(f.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
                if(openCollada(f.getSelectedFile().toString()))
                    canvas.setRedraw();
                else JOptionPane.showMessageDialog(this, "Cannot open " + f.getSelectedFile() + ". Check log for details.", "Cannot open COLLADA file", JOptionPane.ERROR_MESSAGE);
            }
        } else if(e.getSource() == closeAction) {
            close();
            canvas.setRedraw();
        }
    }

    private native boolean openCollada(String filename);
    private native void close();

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
