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
import javax.swing.JButton;
import javax.swing.JColorChooser;
import javax.swing.JFileChooser;
import javax.swing.JFormattedTextField;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSeparator;
import javax.swing.SwingConstants;
import javax.swing.UIManager;

public class JavaViewer extends JFrame implements ActionListener {
    static final long serialVersionUID = 0L;

    private JButton openCollada, openStanford;
    private JButton close;
    private NativeCanvas canvas = new NativeCanvas();

    private class ColoredLabel extends JLabel {
        private static final long serialVersionUID = 0L;

        public ColoredLabel(final int id, Color color) {
            setSize(50, 16);
            setOpaque(true);
            setBackground(color);
            addMouseListener(new MouseAdapter() {
                public void mouseClicked(MouseEvent e) {
                    Color background = JColorChooser.showDialog(null, "Select color...", ColoredLabel.this.getBackground());
                    if(background != null) {
                        ColoredLabel.this.setBackground(background);
                        JavaViewer.this.setLightColor(id, background.getRed()/255.0f, background.getGreen()/255.0f, background.getBlue()/255.0f);
                        canvas.setRedraw();
                    }
                }
            });
        }
    }

    public JavaViewer() {
        super("Magnum/Java Viewer");
        setSize(800, 600);
        add(BorderLayout.CENTER, canvas);
        add(BorderLayout.EAST, new Box(BoxLayout.Y_AXIS) {
            private static final long serialVersionUID = 0L;

            {
                add(openCollada = new JButton("Open COLLADA file..."));
                openCollada.addActionListener(JavaViewer.this);
                add(openStanford = new JButton("Open Stanford PLY file..."));
                openStanford.addActionListener(JavaViewer.this);
                add(close = new JButton("Close file"));
                close.addActionListener(JavaViewer.this);
                add(new JPanel() {
                    private static final long serialVersionUID = 0L;

                    {
                        setLayout(new GridLayout(4, 2));
                        add(new JLabel("Clear color:"));
                        add(new ColoredLabel(3, Color.lightGray));
                        add(new JLabel("Key light:"));
                        add(new ColoredLabel(0, new Color(1.0f, 1.0f, 1.0f)));
                        add(new JLabel("Fill light:"));
                        add(new ColoredLabel(1, new Color(0.2f, 0.0f, 0.0f)));
                        add(new JLabel("Back light:"));
                        add(new ColoredLabel(2, new Color(1.0f, 1.0f, 1.0f)));
                    }
                });
                add(Box.createVerticalStrut(1000));
            }
        });

        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    }

    @Override
    public void actionPerformed(ActionEvent e) {
        if(e.getSource() == openCollada) {
            JFileChooser f = new JFileChooser();
            if(f.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
                if(openCollada(f.getSelectedFile().toString()))
                    canvas.setRedraw();
                else JOptionPane.showMessageDialog(this, "Cannot open " + f.getSelectedFile() + ". Check log for details.", "Cannot open COLLADA file", JOptionPane.ERROR_MESSAGE);
            }
        } else if(e.getSource() == openStanford) {
            JFileChooser f = new JFileChooser();
            if(f.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
                if(openStanford(f.getSelectedFile().toString()))
                    canvas.setRedraw();
                else JOptionPane.showMessageDialog(this, "Cannot open " + f.getSelectedFile() + ". Check log for details.", "Cannot open Stanford PLY file", JOptionPane.ERROR_MESSAGE);
            }
        } else if(e.getSource() == close) {
            close();
            canvas.setRedraw();
        }
    }

    private native boolean openCollada(String filename);
    private native boolean openStanford(String filename);
    private native void close();
    private native void setLightColor(int id, float r, float g, float b);

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
