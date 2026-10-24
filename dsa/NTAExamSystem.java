import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import java.util.Timer;

/**
 * ╔══════════════════════════════════════════════════════════════╗
 *   NTA-Style Online Examination System
 *   Backend : C++ DSA Engine (exam_engine.cpp)
 *   Frontend: Java Swing UI
 *   Communication: Inter-Process via stdin/stdout pipes
 * ╚══════════════════════════════════════════════════════════════╝
 */
public class NTAExamSystem extends JFrame {

    // ── C++ Engine Process ──
    private Process cppProcess;
    private BufferedReader cppReader;
    private BufferedWriter cppWriter;

    // ── Exam State ──
    private int totalQuestions = 0;
    private int currentQuestion = 1;
    private int[] selectedOptions;   // 0 = none selected
    private int[] questionStates;    // mirrors C++ QuestionStatus enum

    // ── Timer ──
    private int remainingSeconds = 60 * 60; // 1 hour
    private Timer examTimer;
    private JLabel timerLabel;

    // ── UI Components ──
    private JLabel questionNumberLabel;
    private JTextArea questionTextArea;
    private JRadioButton[] optionButtons = new JRadioButton[4];
    private ButtonGroup optionGroup;
    private JPanel palettePanel;
    private JButton[] paletteButtons;
    private JLabel lblNotVisited, lblNotAnswered, lblAnswered, lblMarked, lblAnsMarked;

    // ── NTA Color Palette ──
    static final Color NTA_BLUE      = new Color(0x1A, 0x73, 0xE8);
    static final Color NTA_DARK      = new Color(0x2C, 0x3E, 0x50);
    static final Color NTA_GREEN     = new Color(0x27, 0xAE, 0x60);
    static final Color NTA_RED       = new Color(0xE7, 0x4C, 0x3C);
    static final Color NTA_PURPLE    = new Color(0x8E, 0x44, 0xAD);
    static final Color NTA_GRAY      = new Color(0xBD, 0xC3, 0xC7);
    static final Color NTA_BG        = new Color(0xEC, 0xF0, 0xF1);
    static final Color NTA_WHITE     = Color.WHITE;

    // ── Fonts ──
    static final Font FONT_HEADER = new Font("Segoe UI", Font.BOLD, 18);
    static final Font FONT_TITLE  = new Font("Segoe UI", Font.BOLD, 14);
    static final Font FONT_BODY   = new Font("Segoe UI", Font.PLAIN, 14);
    static final Font FONT_SMALL  = new Font("Segoe UI", Font.PLAIN, 12);
    static final Font FONT_TIMER  = new Font("Consolas", Font.BOLD, 20);
    static final Font FONT_BTN    = new Font("Segoe UI", Font.BOLD, 13);

    // ================================================================
    //  CONSTRUCTOR
    // ================================================================
    public NTAExamSystem() {
        super("NTA Online Examination System");
        setDefaultCloseOperation(DO_NOTHING_ON_CLOSE);
        setExtendedState(JFrame.MAXIMIZED_BOTH);
        setMinimumSize(new Dimension(1100, 700));

        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                confirmSubmit();
            }
        });

        startCppEngine();
        totalQuestions = Integer.parseInt(sendCommand("GET_COUNT").trim());
        selectedOptions = new int[totalQuestions + 1];
        questionStates  = new int[totalQuestions + 1];

        buildUI();
        startTimer();
        loadQuestion(1);
        setVisible(true);
    }

    // ================================================================
    //  C++ ENGINE COMMUNICATION
    // ================================================================
    private void startCppEngine() {
        try {
            String os = System.getProperty("os.name").toLowerCase();
            String exePath;
            if (os.contains("win")) {
                exePath = "exam_engine.exe";
            } else {
                exePath = "./exam_engine";
            }

            File exeFile = new File(exePath);
            if (!exeFile.exists()) {
                JOptionPane.showMessageDialog(null,
                    "C++ engine not found!\n\nPlease compile first:\n  g++ -o exam_engine exam_engine.cpp",
                    "Engine Not Found", JOptionPane.ERROR_MESSAGE);
                System.exit(1);
            }

            ProcessBuilder pb = new ProcessBuilder(exeFile.getAbsolutePath());
            pb.directory(exeFile.getParentFile());
            pb.redirectErrorStream(true);
            cppProcess = pb.start();
            cppReader = new BufferedReader(new InputStreamReader(cppProcess.getInputStream()));
            cppWriter = new BufferedWriter(new OutputStreamWriter(cppProcess.getOutputStream()));
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null,
                "Failed to start C++ engine:\n" + e.getMessage(),
                "Error", JOptionPane.ERROR_MESSAGE);
            System.exit(1);
        }
    }

    private synchronized String sendCommand(String cmd) {
        try {
            cppWriter.write(cmd + "\n");
            cppWriter.flush();
            String response = cppReader.readLine();
            return response != null ? response : "";
        } catch (Exception e) {
            e.printStackTrace();
            return "";
        }
    }

    // ================================================================
    //  COLORED BUTTON — works on Windows L&F
    // ================================================================
    private static JButton coloredButton(String text, Color bg, Color fg) {
        JButton btn = new JButton(text) {
            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                if (getModel().isPressed()) {
                    g2.setColor(bg.darker());
                } else if (getModel().isRollover()) {
                    g2.setColor(bg.brighter());
                } else {
                    g2.setColor(bg);
                }
                g2.fillRoundRect(0, 0, getWidth(), getHeight(), 8, 8);
                g2.dispose();
                super.paintComponent(g);
            }
        };
        btn.setFont(FONT_BTN);
        btn.setForeground(fg);
        btn.setContentAreaFilled(false);
        btn.setOpaque(false);
        btn.setBorderPainted(false);
        btn.setFocusPainted(false);
        btn.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        btn.setBorder(BorderFactory.createEmptyBorder(10, 20, 10, 20));
        return btn;
    }

    // Palette button — square, no rounded corners, fully filled
    private static JButton paletteButton(String text, Color bg) {
        JButton btn = new JButton(text) {
            private Color currentBg = bg;

            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setColor(currentBg);
                g2.fillRect(0, 0, getWidth(), getHeight());
                g2.dispose();
                super.paintComponent(g);
            }

            @Override
            public void setBackground(Color c) {
                currentBg = c;
                repaint();
            }

            @Override
            public Color getBackground() {
                return currentBg;
            }
        };
        btn.setFont(FONT_BTN);
        btn.setForeground(Color.BLACK);
        btn.setContentAreaFilled(false);
        btn.setOpaque(false);
        btn.setFocusPainted(false);
        btn.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        btn.setPreferredSize(new Dimension(48, 40));
        btn.setBorder(BorderFactory.createLineBorder(new Color(180, 180, 180), 1));
        return btn;
    }

    // ================================================================
    //  BUILD UI
    // ================================================================
    private void buildUI() {
        setLayout(new BorderLayout());

        add(createHeaderPanel(), BorderLayout.NORTH);

        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,
                createQuestionPanel(), createRightPanel());
        splitPane.setDividerLocation(800);
        splitPane.setResizeWeight(0.75);
        splitPane.setDividerSize(2);
        add(splitPane, BorderLayout.CENTER);

        add(createBottomPanel(), BorderLayout.SOUTH);
    }

    // ──────────────────────────────────────────────
    //  HEADER PANEL
    // ──────────────────────────────────────────────
    private JPanel createHeaderPanel() {
        JPanel header = new JPanel(new BorderLayout());
        header.setBackground(NTA_BLUE);
        header.setBorder(BorderFactory.createEmptyBorder(10, 20, 10, 20));

        JLabel title = new JLabel("  NTA Online Examination System");
        title.setFont(FONT_HEADER);
        title.setForeground(NTA_WHITE);

        timerLabel = new JLabel(formatTime(remainingSeconds));
        timerLabel.setFont(FONT_TIMER);
        timerLabel.setForeground(Color.YELLOW);
        timerLabel.setOpaque(true);
        timerLabel.setBackground(new Color(0, 0, 0, 80));
        timerLabel.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(Color.YELLOW, 2),
            BorderFactory.createEmptyBorder(4, 14, 4, 14)
        ));

        JPanel timerBox = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        timerBox.setOpaque(false);
        JLabel timeLabel = new JLabel("Time Remaining: ");
        timeLabel.setFont(FONT_BODY);
        timeLabel.setForeground(NTA_WHITE);
        timerBox.add(timeLabel);
        timerBox.add(timerLabel);

        header.add(title, BorderLayout.WEST);
        header.add(timerBox, BorderLayout.EAST);
        return header;
    }

    // ──────────────────────────────────────────────
    //  QUESTION PANEL (LEFT)
    // ──────────────────────────────────────────────
    private JPanel createQuestionPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBackground(NTA_WHITE);
        panel.setBorder(BorderFactory.createEmptyBorder(15, 20, 15, 20));

        // Section bar
        JPanel sectionBar = new JPanel(new FlowLayout(FlowLayout.LEFT));
        sectionBar.setBackground(NTA_DARK);
        sectionBar.setBorder(BorderFactory.createEmptyBorder(6, 15, 6, 15));
        JLabel sectionLabel = new JLabel("Section: General Paper  |  Question Type: MCQ  |  Marks: +4 / -1");
        sectionLabel.setFont(FONT_SMALL);
        sectionLabel.setForeground(NTA_WHITE);
        sectionBar.add(sectionLabel);

        // Question number
        questionNumberLabel = new JLabel("Question 1");
        questionNumberLabel.setFont(FONT_TITLE);
        questionNumberLabel.setForeground(NTA_DARK);
        questionNumberLabel.setBorder(BorderFactory.createEmptyBorder(10, 0, 5, 0));

        // Question text
        questionTextArea = new JTextArea();
        questionTextArea.setFont(new Font("Segoe UI", Font.PLAIN, 16));
        questionTextArea.setLineWrap(true);
        questionTextArea.setWrapStyleWord(true);
        questionTextArea.setEditable(false);
        questionTextArea.setBackground(NTA_BG);
        questionTextArea.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(new Color(200, 200, 200)),
            BorderFactory.createEmptyBorder(15, 18, 15, 18)
        ));

        // Options
        JPanel optionsPanel = new JPanel();
        optionsPanel.setLayout(new BoxLayout(optionsPanel, BoxLayout.Y_AXIS));
        optionsPanel.setBackground(NTA_WHITE);
        optionsPanel.setBorder(BorderFactory.createEmptyBorder(15, 10, 10, 10));
        optionGroup = new ButtonGroup();

        String[] labels = {"A", "B", "C", "D"};
        for (int i = 0; i < 4; i++) {
            optionButtons[i] = new JRadioButton();
            optionButtons[i].setFont(new Font("Segoe UI", Font.PLAIN, 15));
            optionButtons[i].setBackground(NTA_WHITE);
            optionButtons[i].setFocusPainted(false);
            final int idx = i;
            optionButtons[i].addActionListener(e -> {
                selectedOptions[currentQuestion] = idx + 1;
            });

            JPanel optRow = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 5));
            optRow.setBackground(NTA_WHITE);
            JLabel lbl = new JLabel(" (" + labels[i] + ") ");
            lbl.setFont(new Font("Segoe UI", Font.BOLD, 15));
            lbl.setForeground(NTA_BLUE);
            optRow.add(lbl);
            optRow.add(optionButtons[i]);
            optionsPanel.add(optRow);

            optionGroup.add(optionButtons[i]);
        }

        // Assemble
        JPanel topPart = new JPanel(new BorderLayout());
        topPart.setBackground(NTA_WHITE);
        topPart.add(sectionBar, BorderLayout.NORTH);

        JPanel questionBody = new JPanel(new BorderLayout());
        questionBody.setBackground(NTA_WHITE);
        questionBody.add(questionNumberLabel, BorderLayout.NORTH);
        questionBody.add(questionTextArea, BorderLayout.CENTER);
        questionBody.add(optionsPanel, BorderLayout.SOUTH);

        topPart.add(questionBody, BorderLayout.CENTER);

        JScrollPane scrollPane = new JScrollPane(topPart);
        scrollPane.setBorder(null);
        scrollPane.getVerticalScrollBar().setUnitIncrement(16);
        panel.add(scrollPane, BorderLayout.CENTER);

        return panel;
    }

    // ──────────────────────────────────────────────
    //  RIGHT PANEL (Palette + Legend)
    // ──────────────────────────────────────────────
    private JPanel createRightPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBackground(NTA_BG);
        panel.setBorder(BorderFactory.createMatteBorder(0, 2, 0, 0, NTA_BLUE));

        // ── Legend ──
        JPanel legendPanel = new JPanel(new GridLayout(5, 1, 4, 4));
        legendPanel.setBackground(NTA_BG);
        legendPanel.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createTitledBorder(
                BorderFactory.createLineBorder(NTA_BLUE),
                " Question Status ",
                TitledBorder.CENTER, TitledBorder.TOP, FONT_TITLE, NTA_BLUE),
            BorderFactory.createEmptyBorder(8, 10, 8, 10)
        ));

        lblNotVisited   = createLegendRow(NTA_GRAY,   "Not Visited",      0);
        lblNotAnswered  = createLegendRow(NTA_RED,    "Not Answered",     0);
        lblAnswered     = createLegendRow(NTA_GREEN,  "Answered",         0);
        lblMarked       = createLegendRow(NTA_PURPLE, "Marked for Review",0);
        lblAnsMarked    = createLegendRow(NTA_PURPLE, "Answered & Marked",0);

        legendPanel.add(lblNotVisited);
        legendPanel.add(lblNotAnswered);
        legendPanel.add(lblAnswered);
        legendPanel.add(lblMarked);
        legendPanel.add(lblAnsMarked);

        // ── Palette Grid ──
        JPanel paletteTitlePanel = new JPanel(new BorderLayout());
        paletteTitlePanel.setBackground(NTA_DARK);
        paletteTitlePanel.setBorder(BorderFactory.createEmptyBorder(8, 10, 8, 10));
        JLabel palTitle = new JLabel("Question Palette", SwingConstants.CENTER);
        palTitle.setFont(FONT_TITLE);
        palTitle.setForeground(NTA_WHITE);
        paletteTitlePanel.add(palTitle);

        palettePanel = new JPanel(new GridLayout(0, 5, 6, 6));
        palettePanel.setBackground(NTA_BG);
        palettePanel.setBorder(BorderFactory.createEmptyBorder(12, 12, 12, 12));
        paletteButtons = new JButton[totalQuestions];

        for (int i = 0; i < totalQuestions; i++) {
            JButton btn = paletteButton(String.valueOf(i + 1), NTA_GRAY);
            final int qNum = i + 1;
            btn.addActionListener(e -> {
                saveCurrentState();
                loadQuestion(qNum);
            });
            paletteButtons[i] = btn;
            palettePanel.add(btn);
        }

        JPanel paletteContainer = new JPanel(new BorderLayout());
        paletteContainer.setBackground(NTA_BG);
        paletteContainer.add(paletteTitlePanel, BorderLayout.NORTH);
        paletteContainer.add(palettePanel, BorderLayout.CENTER);

        panel.add(legendPanel, BorderLayout.NORTH);
        panel.add(paletteContainer, BorderLayout.CENTER);
        return panel;
    }

    private JLabel createLegendRow(Color color, String text, int count) {
        // Custom painted legend label with a colored square
        JLabel lbl = new JLabel("     " + count + "   " + text) {
            @Override
            protected void paintComponent(Graphics g) {
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                g2.setColor(color);
                g2.fillRoundRect(6, (getHeight() - 20) / 2, 24, 20, 5, 5);
                g2.setColor(Color.WHITE);
                g2.setFont(new Font("Segoe UI", Font.BOLD, 10));
                // draw count inside the square
                String num = getText().trim().split("\\s+")[0];
                FontMetrics fm = g2.getFontMetrics();
                int tw = fm.stringWidth(num);
                g2.drawString(num, 6 + (24 - tw) / 2, (getHeight() + fm.getAscent()) / 2 - 2);
                g2.dispose();
                super.paintComponent(g);
            }
        };
        lbl.setFont(FONT_SMALL);
        lbl.setBorder(BorderFactory.createEmptyBorder(2, 32, 2, 5));
        return lbl;
    }

    // ──────────────────────────────────────────────
    //  BOTTOM NAVIGATION PANEL
    // ──────────────────────────────────────────────
    private JPanel createBottomPanel() {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBackground(NTA_DARK);
        panel.setBorder(BorderFactory.createEmptyBorder(8, 20, 8, 20));

        JPanel leftBtns = new JPanel(new FlowLayout(FlowLayout.LEFT, 10, 0));
        leftBtns.setOpaque(false);

        JButton btnMarkReview = coloredButton("Mark for Review", NTA_PURPLE, NTA_WHITE);
        btnMarkReview.addActionListener(e -> markForReview());

        JButton btnClear = coloredButton("Clear Response", new Color(0x95, 0xA5, 0xA6), NTA_WHITE);
        btnClear.addActionListener(e -> clearResponse());

        leftBtns.add(btnMarkReview);
        leftBtns.add(btnClear);

        JPanel centerBtns = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 0));
        centerBtns.setOpaque(false);

        JButton btnPrev = coloredButton("\u25C0  Previous", NTA_BLUE, NTA_WHITE);
        btnPrev.addActionListener(e -> navigatePrev());

        JButton btnNext = coloredButton("Next  \u25B6", NTA_BLUE, NTA_WHITE);
        btnNext.addActionListener(e -> navigateNext());

        JButton btnSaveNext = coloredButton("Save & Next  \u25B6\u25B6", NTA_GREEN, NTA_WHITE);
        btnSaveNext.addActionListener(e -> saveAndNext());

        centerBtns.add(btnPrev);
        centerBtns.add(btnNext);
        centerBtns.add(btnSaveNext);

        JPanel rightBtns = new JPanel(new FlowLayout(FlowLayout.RIGHT, 10, 0));
        rightBtns.setOpaque(false);

        JButton btnSubmit = coloredButton("  Submit Exam  ", NTA_RED, NTA_WHITE);
        btnSubmit.setFont(new Font("Segoe UI", Font.BOLD, 15));
        btnSubmit.addActionListener(e -> confirmSubmit());
        rightBtns.add(btnSubmit);

        panel.add(leftBtns, BorderLayout.WEST);
        panel.add(centerBtns, BorderLayout.CENTER);
        panel.add(rightBtns, BorderLayout.EAST);
        return panel;
    }

    // ================================================================
    //  QUESTION LOADING & STATE MANAGEMENT
    // ================================================================
    private void loadQuestion(int qNum) {
        currentQuestion = qNum;
        String response = sendCommand("GET_Q " + qNum);

        // Format: id|text|opt1~~~opt2~~~opt3~~~opt4|selectedOpt|state
        String[] parts = response.split("\\|");
        if (parts.length < 5) return;

        String qText = parts[1].replace("\\n", "\n");
        String[] options = parts[2].split("~~~");
        int savedOpt = Integer.parseInt(parts[3].trim());
        int state = Integer.parseInt(parts[4].trim());

        questionNumberLabel.setText("Question " + qNum + " of " + totalQuestions);
        questionTextArea.setText(qText);

        optionGroup.clearSelection();
        for (int i = 0; i < 4 && i < options.length; i++) {
            optionButtons[i].setText(options[i].trim());
            if (selectedOptions[qNum] == i + 1) {
                optionButtons[i].setSelected(true);
            } else if (selectedOptions[qNum] == 0 && savedOpt == i + 1) {
                optionButtons[i].setSelected(true);
                selectedOptions[qNum] = savedOpt;
            }
        }

        // If first visit, mark as NOT_ANSWERED
        if (questionStates[qNum] == 0) {
            questionStates[qNum] = 1; // NOT_ANSWERED
        }

        updatePalette();
        updateLegend();
    }

    private void saveCurrentState() {
        int opt = selectedOptions[currentQuestion];
        int state = questionStates[currentQuestion];

        if (opt > 0 && state == 1) {
            state = 2; // ANSWERED
            questionStates[currentQuestion] = state;
        }

        sendCommand("UPDATE " + currentQuestion + " " + opt + " " + state);
    }

    // ──────────────────────────────────────────────
    //  NAVIGATION ACTIONS
    // ──────────────────────────────────────────────
    private void navigateNext() {
        saveCurrentState();
        if (currentQuestion < totalQuestions) {
            loadQuestion(currentQuestion + 1);
        }
    }

    private void navigatePrev() {
        saveCurrentState();
        if (currentQuestion > 1) {
            loadQuestion(currentQuestion - 1);
        }
    }

    private void saveAndNext() {
        int opt = selectedOptions[currentQuestion];
        if (opt > 0) {
            questionStates[currentQuestion] = 2; // ANSWERED
        } else {
            questionStates[currentQuestion] = 1; // NOT_ANSWERED
        }
        saveCurrentState();
        if (currentQuestion < totalQuestions) {
            loadQuestion(currentQuestion + 1);
        } else {
            updatePalette();
            updateLegend();
        }
    }

    private void markForReview() {
        int opt = selectedOptions[currentQuestion];
        if (opt > 0) {
            questionStates[currentQuestion] = 4; // ANSWERED_AND_MARKED
        } else {
            questionStates[currentQuestion] = 3; // MARKED_FOR_REVIEW
        }
        saveCurrentState();
        if (currentQuestion < totalQuestions) {
            loadQuestion(currentQuestion + 1);
        } else {
            updatePalette();
            updateLegend();
        }
    }

    private void clearResponse() {
        optionGroup.clearSelection();
        selectedOptions[currentQuestion] = 0;
        questionStates[currentQuestion] = 1; // NOT_ANSWERED
        saveCurrentState();
        updatePalette();
        updateLegend();
    }

    // ──────────────────────────────────────────────
    //  PALETTE & LEGEND UPDATES
    // ──────────────────────────────────────────────
    private void updatePalette() {
        for (int i = 0; i < totalQuestions; i++) {
            int state = questionStates[i + 1];
            Color c;
            Color fg;
            switch (state) {
                case 1: c = NTA_RED;    fg = Color.WHITE; break;
                case 2: c = NTA_GREEN;  fg = Color.WHITE; break;
                case 3: c = NTA_PURPLE; fg = Color.WHITE; break;
                case 4: c = NTA_PURPLE; fg = Color.WHITE; break;
                default: c = NTA_GRAY;  fg = Color.BLACK; break;
            }
            paletteButtons[i].setBackground(c);
            paletteButtons[i].setForeground(fg);

            // Yellow border on current question
            if (i + 1 == currentQuestion) {
                paletteButtons[i].setBorder(BorderFactory.createLineBorder(Color.ORANGE, 3));
            } else {
                paletteButtons[i].setBorder(BorderFactory.createLineBorder(new Color(180, 180, 180), 1));
            }
        }
    }

    private void updateLegend() {
        int[] counts = new int[5];
        for (int i = 1; i <= totalQuestions; i++) {
            int s = questionStates[i];
            if (s >= 0 && s <= 4) counts[s]++;
        }
        lblNotVisited.setText("     " + counts[0] + "   Not Visited");
        lblNotAnswered.setText("     " + counts[1] + "   Not Answered");
        lblAnswered.setText("     " + counts[2] + "   Answered");
        lblMarked.setText("     " + counts[3] + "   Marked for Review");
        lblAnsMarked.setText("     " + counts[4] + "   Answered & Marked");
    }

    // ──────────────────────────────────────────────
    //  TIMER
    // ──────────────────────────────────────────────
    private void startTimer() {
        examTimer = new Timer();
        examTimer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                remainingSeconds--;
                SwingUtilities.invokeLater(() -> {
                    timerLabel.setText(formatTime(remainingSeconds));
                    if (remainingSeconds <= 300) {
                        timerLabel.setForeground(Color.RED);
                    }
                    if (remainingSeconds <= 0) {
                        examTimer.cancel();
                        autoSubmit();
                    }
                });
            }
        }, 1000, 1000);
    }

    private String formatTime(int seconds) {
        int h = seconds / 3600;
        int m = (seconds % 3600) / 60;
        int s = seconds % 60;
        return String.format(" %02d : %02d : %02d ", h, m, s);
    }

    // ──────────────────────────────────────────────
    //  SUBMISSION
    // ──────────────────────────────────────────────
    private void confirmSubmit() {
        saveCurrentState();

        String summary = sendCommand("GET_SUMMARY");
        String[] parts = summary.trim().split("\\s+");
        String msg = String.format(
            "════════════════════════════════\n" +
            "       SUBMISSION SUMMARY\n" +
            "════════════════════════════════\n\n" +
            "  Not Visited       :  %s\n" +
            "  Not Answered      :  %s\n" +
            "  Answered          :  %s\n" +
            "  Marked for Review :  %s\n" +
            "  Answered & Marked :  %s\n\n" +
            "════════════════════════════════\n" +
            "Are you sure you want to submit?",
            parts.length > 0 ? parts[0] : "0",
            parts.length > 1 ? parts[1] : "0",
            parts.length > 2 ? parts[2] : "0",
            parts.length > 3 ? parts[3] : "0",
            parts.length > 4 ? parts[4] : "0"
        );

        int choice = JOptionPane.showConfirmDialog(this, msg,
            "Confirm Submission", JOptionPane.YES_NO_OPTION,
            JOptionPane.WARNING_MESSAGE);

        if (choice == JOptionPane.YES_OPTION) {
            submitExam();
        }
    }

    private void autoSubmit() {
        JOptionPane.showMessageDialog(this,
            "Time's up! Your exam is being submitted automatically.",
            "Time Over", JOptionPane.WARNING_MESSAGE);
        submitExam();
    }

    private void submitExam() {
        saveCurrentState();
        examTimer.cancel();

        String result = sendCommand("SUBMIT");
        String[] parts = result.split("\\|");
        int score = parts.length > 1 ? Integer.parseInt(parts[1].trim()) : 0;
        int maxScore = totalQuestions * 4;

        try {
            cppWriter.write("EXIT\n");
            cppWriter.flush();
            cppProcess.waitFor();
        } catch (Exception ex) { /* ignore */ }

        showResultScreen(score, maxScore);
    }

    private void showResultScreen(int score, int maxScore) {
        getContentPane().removeAll();
        setLayout(new BorderLayout());

        JPanel resultPanel = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                Graphics2D g2 = (Graphics2D) g;
                g2.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
                GradientPaint gp = new GradientPaint(0, 0, NTA_WHITE, 0, getHeight(), NTA_BG);
                g2.setPaint(gp);
                g2.fillRect(0, 0, getWidth(), getHeight());
            }
        };
        resultPanel.setLayout(new BoxLayout(resultPanel, BoxLayout.Y_AXIS));
        resultPanel.setBorder(BorderFactory.createEmptyBorder(80, 100, 80, 100));

        JLabel titleLbl = new JLabel("Examination Complete");
        titleLbl.setFont(new Font("Segoe UI", Font.BOLD, 36));
        titleLbl.setForeground(NTA_DARK);
        titleLbl.setAlignmentX(CENTER_ALIGNMENT);

        JLabel scoreLbl = new JLabel("Your Score: " + score + " / " + maxScore);
        scoreLbl.setFont(new Font("Segoe UI", Font.BOLD, 30));
        scoreLbl.setForeground(score >= 0 ? NTA_GREEN : NTA_RED);
        scoreLbl.setAlignmentX(CENTER_ALIGNMENT);

        JLabel percentLbl = new JLabel(
            String.format("Percentage: %.1f%%", (score * 100.0) / maxScore));
        percentLbl.setFont(new Font("Segoe UI", Font.PLAIN, 22));
        percentLbl.setForeground(NTA_DARK);
        percentLbl.setAlignmentX(CENTER_ALIGNMENT);

        JButton exitBtn = coloredButton("  Close Application  ", NTA_BLUE, NTA_WHITE);
        exitBtn.setFont(new Font("Segoe UI", Font.BOLD, 16));
        exitBtn.setAlignmentX(CENTER_ALIGNMENT);
        exitBtn.addActionListener(e -> System.exit(0));

        resultPanel.add(Box.createVerticalGlue());
        resultPanel.add(titleLbl);
        resultPanel.add(Box.createVerticalStrut(25));
        resultPanel.add(scoreLbl);
        resultPanel.add(Box.createVerticalStrut(12));
        resultPanel.add(percentLbl);
        resultPanel.add(Box.createVerticalStrut(40));
        resultPanel.add(exitBtn);
        resultPanel.add(Box.createVerticalGlue());

        add(resultPanel, BorderLayout.CENTER);
        revalidate();
        repaint();
    }

    // ================================================================
    //  MAIN
    // ================================================================
    public static void main(String[] args) {
        // Use cross-platform L&F so colors render properly
        try {
            UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
        } catch (Exception e) { /* fallback */ }

        SwingUtilities.invokeLater(NTAExamSystem::new);
    }
}
