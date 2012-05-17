package cz.mosra.magnum.JavaViewer;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JColorChooser;
import javax.swing.JFileChooser;
import javax.swing.JFormattedTextField;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSeparator;
import javax.swing.SwingConstants;
import javax.swing.UIManager;

public class JavaViewer extends JFrame implements ActionListener {
    static final long serialVersionUID = 0L;

    private JMenuItem openColladaAction;
    private JMenuItem closeAction;
    private NativeCanvas canvas = new NativeCanvas();

    private class ColoredLabel extends JLabel {
        private static final long serialVersionUID = 0L;

        public ColoredLabel(Color color) {
            setSize(50, 16);
            setOpaque(true);
            setBackground(color);
            addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent e) {
                    Color background = JColorChooser.showDialog(null, "Select color...", ColoredLabel.this.getBackground());
                    if(background != null) {
                        ColoredLabel.this.setBackground(background);
                    }
                }
            });
        }
    }

    private class LightControl extends JPanel {
        private static final long serialVersionUID = 0L;

        private JFormattedTextField x_, y_, z_;

        public LightControl(float x, float y, float z, Color color) {
            setLayout(new GridLayout(4, 2));
            add(new JLabel("X:"));
            x_ = new JFormattedTextField();
            x_.setValue(x);
            add(x_);
            add(new JLabel("Y:"));
            y_ = new JFormattedTextField();
            y_.setValue(1.0f);
            add(y_);
            add(new JLabel("Z:"));
            z_ = new JFormattedTextField();
            z_.setValue(1.0f);
            add(z_);
            add(new JLabel("Color:"));
            add(new ColoredLabel(color));
        }
    }

    private class SceneControls extends Box {
        private static final long serialVersionUID = 0L;

        SceneControls() {
            super(BoxLayout.Y_AXIS);
            add(new JPanel() {
                private static final long serialVersionUID = 0L;

                {
                    setLayout(new GridLayout(2, 2));
                    add(new JLabel("Clear color:"));
                    add(new ColoredLabel(Color.lightGray));
                    add(new JLabel("Ambient light color:"));
                    add(new ColoredLabel(Color.black));
                }
            });
            add(new JSeparator(SwingConstants.HORIZONTAL));
            add(new JLabel("<html><strong>Key light</strong></html>"));
            add(new LightControl(0.5f, 1.0f, 1.0f, Color.white));
            add(new JSeparator(SwingConstants.HORIZONTAL));
            add(new JLabel("<html><strong>Fill light</strong></html>"));
            add(new LightControl(0.6f, 1.0f, 1.0f, Color.gray));
            add(new JSeparator(SwingConstants.HORIZONTAL));
            add(new JLabel("<html><strong>Back light</strong></html>"));
            add(new LightControl(0.0f, 1.0f, -1.0f, Color.yellow));
            add(Box.createVerticalStrut(1000));
        }
    }

    public JavaViewer() {
        super("Magnum/Java Viewer");
        setSize(800, 600);
        add(BorderLayout.CENTER, canvas);
        add(BorderLayout.EAST, new SceneControls());

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
