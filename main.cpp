#include <includes.h>

class AnimatedTitle : public QLabel {
    Q_OBJECT
    Q_PROPERTY(QString displayText READ displayText WRITE setDisplayText NOTIFY textChanged)

public:
    explicit AnimatedTitle(QWidget *parent = nullptr) : QLabel(parent) {
        m_words << "современный" << "удобный" << "минималистичный" << "PawniX";

        setupWordFonts();

        setFont(m_wordFonts.first());

        QFont titleFont;
        titleFont.setPointSize(24);
        titleFont.setBold(true);
        titleFont.setFamily("Segoe UI");
        setFont(titleFont);

        m_normalColor = QColor(212, 212, 212);

        setAlignment(Qt::AlignCenter);

        m_timer = new QTimer(this);
        m_timer->setInterval(100);
        connect(m_timer, &QTimer::timeout, this, &AnimatedTitle::updateAnimation);

        QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
        shadowEffect->setBlurRadius(20);
        shadowEffect->setColor(m_glowColor);
        shadowEffect->setOffset(0, 0);
        setGraphicsEffect(shadowEffect);

        m_fullText = m_words.first();
        setText("");
    }

    ~AnimatedTitle() {
        stopAnimation();
    }

    void startAnimation() {
        if (!m_timer->isActive()) {
            m_currentIndex = 0;
            m_currentWordIndex = 0;
            setFont(m_wordFonts.first());
            m_isTyping = true;
            m_isErasing = false;
            m_fullText = m_words.first();
            setText("");
            m_timer->start();
        }
    }

    void stopAnimation() {
        m_timer->stop();
    }

    void setupWordFonts() {
        m_wordFonts.clear();

        QFont font1;
        font1.setFamily("Hasklig");
        font1.setPointSize(24);
        font1.setBold(true);
        m_wordFonts.append(font1);

        QFont font2;
        font2.setFamily("Fira Code");
        font2.setPointSize(24);
        font2.setBold(true);
        m_wordFonts.append(font2);

        QFont font3;
        font3.setFamily("Arial");
        font3.setPointSize(24);
        font3.setBold(true);
        m_wordFonts.append(font3);

        QFont font4;
        font4.setFamily("Segoe UI");
        font4.setPointSize(24);
        font4.setBold(true);
        m_wordFonts.append(font4);
    }

    void setWordFont(int wordIndex, const QFont& font) {
        if (wordIndex >= 0 && wordIndex < m_wordFonts.size()) {
            m_wordFonts[wordIndex] = font;
            if (m_currentWordIndex == wordIndex) {
                setFont(font);
            }
        }
    }

    void setWordFonts(const QVector<QFont>& fonts) {
        if (fonts.size() == m_words.size()) {
            m_wordFonts = fonts;
            if (m_timer->isActive()) {
                setFont(m_wordFonts[m_currentWordIndex]);
            }
        }
    }

signals:
    void textChanged(const QString &text);

public slots:
    void setDisplayText(const QString &text) {
        if (m_displayText != text) {
            m_displayText = text;
            setText(text);
            emit textChanged(text);
        }
    }

    QString displayText() const { return m_displayText; }

private slots:
    void updateAnimation() {
        if (m_isTyping) {
            if (m_currentIndex <= m_fullText.length()) {
                QString displayText = m_fullText.left(m_currentIndex);
                setText(displayText);
                m_currentIndex++;

                if (m_currentIndex > m_fullText.length() * 0.7) {
                    m_timer->setInterval(70);
                }
            } else {
                m_isTyping = false;
                m_timer->setInterval(3500);
            }
        }
        else if (m_isErasing) {
            if (m_currentIndex >= 0) {
                QString displayText = m_fullText.left(m_currentIndex);
                setText(displayText);
                m_currentIndex--;

                if (m_currentIndex < m_fullText.length() * 0.3) {
                    m_timer->setInterval(50);
                }
            } else {
                m_isErasing = false;
                m_currentWordIndex = (m_currentWordIndex + 1) % m_words.size();
                m_fullText = m_words[m_currentWordIndex];
                m_isTyping = true;
                m_currentIndex = 0;
                m_timer->setInterval(100);

                if (m_currentWordIndex < m_wordFonts.size()) {
                    setFont(m_wordFonts[m_currentWordIndex]);
                }

                QGraphicsDropShadowEffect *shadowEffect =
                    qobject_cast<QGraphicsDropShadowEffect*>(graphicsEffect());
                if (shadowEffect) {
                    if (m_currentWordIndex == 3) {
                        m_glowColor = QColor(47, 38, 145);
                    } else if (m_currentWordIndex == 0) {
                        m_glowColor = QColor(0, 0, 0);
                    } else if (m_currentWordIndex == 1) {
                        m_glowColor = QColor(0, 0, 0);
                    } else {
                        m_glowColor = QColor(0, 0, 0);
                    }
                    shadowEffect->setColor(m_glowColor);
                }
            }
        } else {
            m_isErasing = true;
            m_currentIndex = m_fullText.length();
            m_timer->setInterval(80);
        }
    }

private:
    QString m_displayText;
    QString m_fullText;
    int m_currentIndex = 0;
    int m_currentWordIndex = 0;
    bool m_isTyping = true;
    bool m_isErasing = false;

    QTimer *m_timer;
    QList<QString> m_words;
    QVector<QFont> m_wordFonts;

    QColor m_normalColor;
    QColor m_glowColor;
};

class PawnHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit PawnHighlighter(QTextDocument *parent = nullptr) : QSyntaxHighlighter(parent) {
        setupDefaultColors();
        setupRules();
    }

    void setThemeColors(const QColor& keyword, const QColor& type, const QColor& preprocessor,
                        const QColor& comment, const QColor& string, const QColor& number,
                        const QColor& constant, const QColor& op) {
        keywordFormat.setForeground(keyword);
        typeFormat.setForeground(type);
        preprocessorFormat.setForeground(preprocessor);
        singleLineCommentFormat.setForeground(comment);
        multiLineCommentFormat.setForeground(comment);
        quotationFormat.setForeground(string);
        numberFormat.setForeground(number);
        constantFormat.setForeground(constant);
        operatorFormat.setForeground(op);

        rehighlight();
    }

protected:
    void highlightBlock(const QString &text) override {
        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(commentStartExpression);
        }

        while (startIndex >= 0) {
            QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
            int endIndex = match.capturedStart();
            int commentLength = 0;

            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + match.capturedLength();
            }

            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
        }

        for (const HighlightRule &rule : highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat typeFormat;
    QTextCharFormat preprocessorFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat constantFormat;
    QTextCharFormat operatorFormat;

    void setupRules() {
        QStringList keywordPatterns;
        keywordPatterns << "\\bassert\\b" << "\\bbreak\\b" << "\\bcase\\b"
                        << "\\bconst\\b" << "\\bcontinue\\b" << "\\bdefault\\b"
                        << "\\bdo\\b" << "\\belse\\b" << "\\benum\\b"
                        << "\\bfor\\b" << "\\bforward\\b" << "\\bfunctag\\b"
                        << "\\bgoto\\b" << "\\bif\\b" << "\\bnative\\b"
                        << "\\bnew\\b" << "\\boperator\\b" << "\\bpublic\\b"
                        << "\\breturn\\b" << "\\bsizeof\\b" << "\\bstatic\\b"
                        << "\\bstock\\b" << "\\bswitch\\b" << "\\btagof\\b"
                        << "\\bwhile\\b" << "\\bdefined\\b";

        for (const QString &pattern : keywordPatterns) {
            HighlightRule rule;
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QStringList typePatterns;
        typePatterns << "\\bFloat\\b" << "\\bFloat\\s*:"
                     << "\\bbool\\b" << "\\bbool\\s*:"
                     << "\\btrue\\b" << "\\bfalse\\b"
                     << "\\bchar\\b" << "\\bchar\\s*:"
                     << "\\bint\\b" << "\\bint\\s*:"
                     << "\\blong\\b" << "\\blong\\s*:"
                     << "\\bshort\\b" << "\\bshort\\s*:"
                     << "\\bunsigned\\b" << "\\bunsigned\\s*:";

        for (const QString &pattern : typePatterns) {
            HighlightRule rule;
            rule.pattern = QRegularExpression(pattern);
            rule.format = typeFormat;
            highlightingRules.append(rule);
        }

        HighlightRule preprocessorRule;
        preprocessorRule.pattern = QRegularExpression("#\\s*\\w+");
        preprocessorRule.format = preprocessorFormat;
        highlightingRules.append(preprocessorRule);

        HighlightRule stringRule;
        stringRule.pattern = QRegularExpression("\".*?\"");
        stringRule.format = quotationFormat;
        highlightingRules.append(stringRule);

        HighlightRule numberRule;
        numberRule.pattern = QRegularExpression("\\b\\d+\\.?\\d*\\b");
        numberRule.format = numberFormat;
        highlightingRules.append(numberRule);

        HighlightRule constantRule;
        constantRule.pattern = QRegularExpression("\\b[A-Z_][A-Z0-9_]*\\b");
        constantRule.format = constantFormat;
        highlightingRules.append(constantRule);

        HighlightRule operatorRule;
        operatorRule.pattern = QRegularExpression("[\\+\\-\\*/%=&|^~<>!]+");
        operatorRule.format = operatorFormat;
        highlightingRules.append(operatorRule);

        HighlightRule commentRule;
        commentRule.pattern = QRegularExpression("//[^\n]*");
        commentRule.format = singleLineCommentFormat;
        highlightingRules.append(commentRule);

        commentStartExpression = QRegularExpression("/\\*");
        commentEndExpression = QRegularExpression("\\*/");
    }

    void setupDefaultColors() {
        keywordFormat.setForeground(QColor(86, 156, 214));
        keywordFormat.setFontWeight(QFont::Bold);

        typeFormat.setForeground(QColor(78, 201, 176));
        typeFormat.setFontItalic(true);

        preprocessorFormat.setForeground(QColor(197, 134, 192));

        singleLineCommentFormat.setForeground(QColor(106, 153, 85));
        multiLineCommentFormat.setForeground(QColor(106, 153, 85));

        quotationFormat.setForeground(QColor(206, 145, 120));

        numberFormat.setForeground(QColor(181, 206, 168));

        constantFormat.setForeground(QColor(79, 193, 255));
        constantFormat.setFontWeight(QFont::Bold);

        operatorFormat.setForeground(QColor(212, 212, 212));
    }
};

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr) : QPlainTextEdit(parent) {
        lineNumberArea = new LineNumberArea(this);

        QPalette editorPalette = palette();
        editorPalette.setColor(QPalette::Base, QColor(30, 30, 30));
        editorPalette.setColor(QPalette::Text, QColor(212, 212, 212));
        editorPalette.setColor(QPalette::Highlight, QColor(38, 79, 120));
        editorPalette.setColor(QPalette::HighlightedText, Qt::white);
        setPalette(editorPalette);

        highlighter = new PawnHighlighter(document());
        setLineWrapMode(QPlainTextEdit::NoWrap);

        connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
        connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
        connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

        updateLineNumberAreaWidth(0);
        highlightCurrentLine();

        document()->setDocumentMargin(0);
        setCursorWidth(2);
    }

    void setEditorFont(const QFont &font) {
        setFont(font);
        setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
        updateLineNumberAreaWidth(0);
        update();
    }

    PawnHighlighter* getHighlighter() const { return highlighter; }

    void updateColors(const QColor& background, const QColor& text, const QColor& highlight,
                      const QColor& highlightText, const QColor& lineNumbers) {
        QPalette editorPalette = palette();
        editorPalette.setColor(QPalette::Base, background);
        editorPalette.setColor(QPalette::Text, text);
        editorPalette.setColor(QPalette::Highlight, highlight);
        editorPalette.setColor(QPalette::HighlightedText, highlightText);
        setPalette(editorPalette);

        lineNumberColor = lineNumbers;
        update();
    }

    void lineNumberAreaPaintEvent(QPaintEvent *event) {
        QPainter painter(lineNumberArea);
        painter.fillRect(event->rect(), QColor(37, 37, 38));

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(lineNumberColor);
                painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                                 Qt::AlignRight, number);
            }

            block = block.next();
            top = bottom;
            bottom = top + qRound(blockBoundingRect(block).height());
            ++blockNumber;
        }
    }

    int lineNumberAreaWidth() {
        int digits = 1;
        int max = qMax(1, blockCount());
        while (max >= 10) {
            max /= 10;
            ++digits;
        }
        return 10 + fontMetrics().horizontalAdvance('9') * digits;
    }

    void findText(const QString &text, bool caseSensitive = false, bool wholeWords = false) {
        if (text.isEmpty()) {
            extraSelections.clear();
            setExtraSelections(extraSelections);
            return;
        }

        QTextDocument *doc = document();
        QTextCursor cursor(doc);
        QList<QTextEdit::ExtraSelection> selections;

        QTextCharFormat fmt;
        fmt.setBackground(QColor(255, 235, 59));
        fmt.setForeground(Qt::black);

        QRegularExpression regex;
        if (wholeWords) {
            regex = QRegularExpression("\\b" + QRegularExpression::escape(text) + "\\b");
        } else {
            regex = QRegularExpression(QRegularExpression::escape(text));
        }

        if (!caseSensitive) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }

        cursor = doc->find(regex, 0);
        while (!cursor.isNull()) {
            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            selection.format = fmt;
            selections.append(selection);
            cursor = doc->find(regex, cursor);
        }

        setExtraSelections(selections);
    }

    void replaceText(const QString &searchText, const QString &replaceText,
                     bool caseSensitive = false, bool wholeWords = false) {
        QTextDocument *doc = document();
        QTextCursor cursor(doc);

        QRegularExpression regex;
        if (wholeWords) {
            regex = QRegularExpression("\\b" + QRegularExpression::escape(searchText) + "\\b");
        } else {
            regex = QRegularExpression(QRegularExpression::escape(searchText));
        }

        if (!caseSensitive) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }

        cursor = doc->find(regex, 0);
        while (!cursor.isNull()) {
            cursor.insertText(replaceText);
            cursor = doc->find(regex, cursor);
        }
    }

    void goToLine(int lineNumber) {
        QTextDocument *doc = document();
        QTextBlock block = doc->findBlockByLineNumber(lineNumber - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            setTextCursor(cursor);
            centerCursor();
        }
    }

    void toggleBookmark(int line = -1) {
        if (line == -1) {
            line = textCursor().blockNumber() + 1;
        }

        if (bookmarks.contains(line)) {
            bookmarks.remove(line);
        } else {
            bookmarks.insert(line);
        }

        updateBookmarkDisplay();
        emit bookmarkChanged(line, bookmarks.contains(line));
    }

    void clearBookmark(int line) {
        bookmarks.remove(line);
        updateBookmarkDisplay();
    }

    bool hasBookmark(int line) const {
        return bookmarks.contains(line);
    }

    void nextBookmark() {
        if (bookmarks.isEmpty()) return;

        int currentLine = textCursor().blockNumber() + 1;
        int nextLine = -1;

        for (int line : bookmarks) {
            if (line > currentLine) {
                if (nextLine == -1 || line < nextLine) {
                    nextLine = line;
                }
            }
        }

        if (nextLine == -1) {
            int minLine = *bookmarks.begin();
            for (int line : bookmarks) {
                if (line < minLine) minLine = line;
            }
            nextLine = minLine;
        }

        if (nextLine > 0) {
            goToLine(nextLine);
        }
    }

    void prevBookmark() {
        if (bookmarks.isEmpty()) return;

        int currentLine = textCursor().blockNumber() + 1;
        int prevLine = -1;

        for (int line : bookmarks) {
            if (line < currentLine) {
                if (prevLine == -1 || line > prevLine) {
                    prevLine = line;
                }
            }
        }

        if (prevLine == -1) {
            int maxLine = *bookmarks.begin();
            for (int line : bookmarks) {
                if (line > maxLine) maxLine = line;
            }
            prevLine = maxLine;
        }

        if (prevLine > 0) {
            goToLine(prevLine);
        }
    }

    QList<int> getBookmarks() const {
        return QList<int>(bookmarks.begin(), bookmarks.end());
    }

    void highlightLine(int line, const QColor &color) {
        QTextDocument *doc = document();
        QTextBlock block = doc->findBlockByLineNumber(line - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

            QTextCharFormat fmt;
            fmt.setBackground(color);

            QTextEdit::ExtraSelection selection;
            selection.cursor = cursor;
            selection.format = fmt;

            lineHighlights[line] = selection;
            updateExtraSelections();
        }
    }

    void clearLineHighlight(int line) {
        lineHighlights.remove(line);
        updateExtraSelections();
    }

    void highlightMatchingParentheses() {
        QTextCursor cursor = textCursor();
        if (cursor.hasSelection()) return;

        int position = cursor.position();
        QTextDocument *doc = document();
        QChar currentChar = doc->characterAt(position);
        QChar previousChar = doc->characterAt(position - 1);

        QList<QTextEdit::ExtraSelection> selections;
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(38, 79, 120));

        if (currentChar == ')' || currentChar == '}' || currentChar == ']') {
            QChar matchingChar;
            if (currentChar == ')') matchingChar = '(';
            else if (currentChar == '}') matchingChar = '{';
            else matchingChar = '[';

            int depth = 1;
            for (int i = position - 1; i >= 0; --i) {
                QChar ch = doc->characterAt(i);
                if (ch == currentChar) depth++;
                else if (ch == matchingChar) {
                    depth--;
                    if (depth == 0) {
                        QTextCursor matchCursor = cursor;
                        matchCursor.setPosition(i);
                        matchCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                        selection.cursor = matchCursor;
                        selections.append(selection);
                        break;
                    }
                }
            }
        }

        if (previousChar == '(' || previousChar == '{' || previousChar == '[') {
            QChar matchingChar;
            if (previousChar == '(') matchingChar = ')';
            else if (previousChar == '{') matchingChar = '}';
            else matchingChar = ']';

            int depth = 1;
            for (int i = position; i < doc->characterCount(); ++i) {
                QChar ch = doc->characterAt(i);
                if (ch == previousChar) depth++;
                else if (ch == matchingChar) {
                    depth--;
                    if (depth == 0) {
                        QTextCursor matchCursor = cursor;
                        matchCursor.setPosition(i);
                        matchCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                        selection.cursor = matchCursor;
                        selections.append(selection);
                        break;
                    }
                }
            }
        }

        if (!selections.isEmpty()) {
            QTextEdit::ExtraSelection currentSelection;
            currentSelection.format.setBackground(QColor(38, 79, 120));
            currentSelection.cursor = cursor;
            currentSelection.cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
            selections.append(currentSelection);
        }

        setExtraSelections(selections);
    }

    void zoomIn(int range = 1) {
        QFont f = font();
        f.setPointSize(f.pointSize() + range);
        setEditorFont(f);
    }

    void zoomOut(int range = 1) {
        QFont f = font();
        int newSize = f.pointSize() - range;
        if (newSize > 6) {
            f.setPointSize(newSize);
            setEditorFont(f);
        }
    }

    void zoomReset() {
        QFont f = font();
        f.setPointSize(12);
        setEditorFont(f);
    }

    int wordCount() const {
        QString text = toPlainText();
        return text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    }

    int charCount() const {
        return toPlainText().length();
    }

    int lineCount() const {
        return document()->blockCount();
    }

signals:
    void bookmarkChanged(int line, bool added);

protected:
    void resizeEvent(QResizeEvent *event) override {
        QPlainTextEdit::resizeEvent(event);
        QRect cr = contentsRect();
        lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Tab && completer && completer->popup()->isVisible()) {
            completer->complete();
            return;
        }

        QPlainTextEdit::keyPressEvent(event);
        if (event->text().contains('(') || event->text().contains(')') ||
            event->text().contains('{') || event->text().contains('}') ||
            event->text().contains('[') || event->text().contains(']')) {
            highlightMatchingParentheses();
        }
    }

private slots:
    void updateLineNumberAreaWidth(int newBlockCount) {
        Q_UNUSED(newBlockCount);
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }

    void updateLineNumberArea(const QRect &rect, int dy) {
        if (dy) {
            lineNumberArea->scroll(0, dy);
        } else {
            lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
        }
    }

    void highlightCurrentLine() {
        QList<QTextEdit::ExtraSelection> extraSelections;

        if (!isReadOnly()) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(QColor(45, 45, 48));
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }

        for (const auto &selection : lineHighlights) {
            extraSelections.append(selection);
        }

        setExtraSelections(extraSelections);
    }

    void updateBookmarkDisplay() {
    }

    void updateExtraSelections() {
        highlightCurrentLine();
    }

private:
    class LineNumberArea : public QWidget {
    public:
        LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

        QSize sizeHint() const override {
            return QSize(codeEditor->lineNumberAreaWidth(), 0);
        }

    protected:
        void paintEvent(QPaintEvent *event) override {
            codeEditor->lineNumberAreaPaintEvent(event);
        }

    private:
        CodeEditor *codeEditor;
    };

    LineNumberArea *lineNumberArea;
    PawnHighlighter *highlighter;
    QCompleter *completer = nullptr;
    QSet<int> bookmarks;
    QMap<int, QTextEdit::ExtraSelection> lineHighlights;
    QList<QTextEdit::ExtraSelection> extraSelections;
    QColor lineNumberColor = QColor(133, 133, 133);
};

class StartPage : public QWidget {
    Q_OBJECT

public:
    explicit StartPage(QWidget *parent = nullptr) : QWidget(parent) {
        setupUI();
        setupSignals();
    }

    ~StartPage() {
        if (titleLabel) {
            titleLabel->stopAnimation();
        }
    }

    void updateRecentFiles(const QStringList &files) {
        recentFilesList->clear();
        for (const QString &file : files) {
            QFileInfo info(file);
            QListWidgetItem *item = new QListWidgetItem(
                QIcon::fromTheme("text-x-generic"),
                info.fileName() + "\n" + info.absolutePath()
                );
            item->setData(Qt::UserRole, file);
            recentFilesList->addItem(item);
        }
    }

    void startTitleAnimation() {
        if (titleLabel) {
            titleLabel->startAnimation();
        }
    }

    void stopTitleAnimation() {
        if (titleLabel) {
            titleLabel->stopAnimation();
        }
    }

signals:
    void createNewFile();
    void openFile();
    void openFolder();
    void openRecentFile(const QString &filePath);

private slots:
    void onRecentFileClicked(QListWidgetItem *item) {
        if (item) {
            emit openRecentFile(item->data(Qt::UserRole).toString());
        }
    }

    void onCreateFileClicked() {
        emit createNewFile();
    }

    void onOpenFileClicked() {
        emit openFile();
    }

    void onOpenFolderClicked() {
        emit openFolder();
    }

private:
    void setupUI() {
        mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(50, 50, 50, 50);
        mainLayout->setSpacing(25);

        titleLabel = new AnimatedTitle(this);
        mainLayout->addWidget(titleLabel);
        mainLayout->addSpacing(40);

        QWidget *buttonContainer = new QWidget(this);
        QHBoxLayout *buttonContainerLayout = new QHBoxLayout(buttonContainer);
        buttonContainerLayout->setContentsMargins(0, 0, 0, 0);
        buttonContainerLayout->setSpacing(20);

        newFileButton = new QPushButton(" &Новый файл");
        openFileButton = new QPushButton(" &Открыть файл");
        openFolderButton = new QPushButton(" &Открыть папку");

        QString buttonStyle =
            "QPushButton {"
            "    padding: 12px 24px;"
            "    font-size: 13px;"
            "    font-weight: 600;"
            "    min-width: 160px;"
            "    min-height: 45px;"
            "    background-color: #2D2D30;"
            "    color: #D4D4D4;"
            "    border: 1px solid #3E3E42;"
            "    border-radius: 6px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #404040;"
            "    border-color: #007acc;"
            "    transform: translateY(-2px);"
            "    box-shadow: 0 4px 8px rgba(0, 122, 204, 0.3);"
            "}"
            "QPushButton:pressed {"
            "    background-color: #252526;"
            "    transform: translateY(0px);"
            "    border-color: #005a9e;"
            "}";

        newFileButton->setStyleSheet(buttonStyle);
        openFileButton->setStyleSheet(buttonStyle);
        openFolderButton->setStyleSheet(buttonStyle);

        newFileButton->setIcon(QIcon::fromTheme("document-new"));
        openFileButton->setIcon(QIcon::fromTheme("document-open"));
        openFolderButton->setIcon(QIcon::fromTheme("folder-open"));

        buttonContainerLayout->addStretch();
        buttonContainerLayout->addWidget(newFileButton);
        buttonContainerLayout->addWidget(openFileButton);
        buttonContainerLayout->addWidget(openFolderButton);
        buttonContainerLayout->addStretch();

        mainLayout->addWidget(buttonContainer);
        mainLayout->addSpacing(40);

        QLabel *recentLabel = new QLabel("Недавние файлы:");
        recentLabel->setStyleSheet("color: #D4D4D4; font-size: 18px; font-weight: bold; margin: 10px 0;");
        recentLabel->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(recentLabel);

        recentFilesList = new QListWidget();
        recentFilesList->setStyleSheet(
            "QListWidget {"
            "    background-color: #252526;"
            "    color: #D4D4D4;"
            "    border: 1px solid #3E3E42;"
            "    border-radius: 6px;"
            "    padding: 5px;"
            "}"
            "QListWidget::item {"
            "    padding: 12px;"
            "    border-bottom: 1px solid #3E3E42;"
            "    font-size: 12px;"
            "}"
            "QListWidget::item:hover {"
            "    background-color: #404040;"
            "}"
            "QListWidget::item:selected {"
            "    background-color: #094771;"
            "}"
            "QListWidget::item:last {"
            "    border-bottom: none;"
            "}"
            );
        recentFilesList->setMinimumHeight(180);
        recentFilesList->setMaximumHeight(250);

        QFont listFont;
        listFont.setPointSize(11);
        recentFilesList->setFont(listFont);

        mainLayout->addWidget(recentFilesList);
        mainLayout->addStretch();

        QTimer::singleShot(500, this, [this]() {
            titleLabel->startAnimation();
        });
    }

    void setupSignals() {
        connect(newFileButton, &QPushButton::clicked, this, &StartPage::onCreateFileClicked);
        connect(openFileButton, &QPushButton::clicked, this, &StartPage::onOpenFileClicked);
        connect(openFolderButton, &QPushButton::clicked, this, &StartPage::onOpenFolderClicked);
        connect(recentFilesList, &QListWidget::itemClicked, this, &StartPage::onRecentFileClicked);
    }

private:
    AnimatedTitle *titleLabel;
    QListWidget *recentFilesList;

    QPushButton *newFileButton;
    QPushButton *openFileButton;
    QPushButton *openFolderButton;

    QVBoxLayout *mainLayout;
};

class FindDialog : public QDialog {
    Q_OBJECT

public:
    explicit FindDialog(CodeEditor *editor, QWidget *parent = nullptr)
        : QDialog(parent), codeEditor(editor), currentPosition(0) {
        setupUI();
        setupSignals();
    }

    QString searchText() const { return searchEdit->text(); }
    QString replaceText() const { return replaceEdit->text(); }
    bool caseSensitive() const { return caseCheckBox->isChecked(); }
    bool wholeWords() const { return wholeWordCheckBox->isChecked(); }
    bool regex() const { return regexCheckBox->isChecked(); }
    bool backward() const { return backwardCheckBox->isChecked(); }

private slots:
    void findNext() {
        if (searchText().isEmpty()) return;

        codeEditor->findText(searchText(), caseSensitive(), wholeWords());
        statusLabel->setText("Поиск выполнен");
    }

    void findPrevious() {
        if (searchText().isEmpty()) return;

        statusLabel->setText("Поиск выполнен");
    }

    void replace() {
        if (searchText().isEmpty()) return;

        QTextCursor cursor = codeEditor->textCursor();
        if (cursor.hasSelection() && cursor.selectedText() == searchText()) {
            cursor.insertText(replaceText());
        }
        findNext();
    }

    void replaceAll() {
        if (searchText().isEmpty()) return;

        codeEditor->replaceText(searchText(), replaceText(), caseSensitive(), wholeWords());
        statusLabel->setText("Все вхождения заменены");
    }

    void updateUI() {
        bool hasText = !searchEdit->text().isEmpty();
        findNextButton->setEnabled(hasText);
        findPrevButton->setEnabled(hasText);
        replaceButton->setEnabled(hasText);
        replaceAllButton->setEnabled(hasText);
    }

private:
    void setupUI() {
        setWindowTitle("Поиск и замена");
        setFixedSize(400, 250);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QHBoxLayout *searchLayout = new QHBoxLayout();
        searchLayout->addWidget(new QLabel("Найти:"));
        searchEdit = new QLineEdit();
        searchEdit->setPlaceholderText("Введите текст для поиска...");
        searchLayout->addWidget(searchEdit);

        QHBoxLayout *replaceLayout = new QHBoxLayout();
        replaceLayout->addWidget(new QLabel("Заменить на:"));
        replaceEdit = new QLineEdit();
        replaceEdit->setPlaceholderText("Введите текст для замены...");
        replaceLayout->addWidget(replaceEdit);

        QGridLayout *optionsLayout = new QGridLayout();
        caseCheckBox = new QCheckBox("Учитывать регистр");
        wholeWordCheckBox = new QCheckBox("Целое слово");
        regexCheckBox = new QCheckBox("Регулярное выражение");
        backwardCheckBox = new QCheckBox("Назад");

        optionsLayout->addWidget(caseCheckBox, 0, 0);
        optionsLayout->addWidget(wholeWordCheckBox, 0, 1);
        optionsLayout->addWidget(regexCheckBox, 1, 0);
        optionsLayout->addWidget(backwardCheckBox, 1, 1);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        findNextButton = new QPushButton("Найти далее");
        findPrevButton = new QPushButton("Найти назад");
        replaceButton = new QPushButton("Заменить");
        replaceAllButton = new QPushButton("Заменить все");
        closeButton = new QPushButton("Закрыть");

        buttonLayout->addWidget(findNextButton);
        buttonLayout->addWidget(findPrevButton);
        buttonLayout->addWidget(replaceButton);
        buttonLayout->addWidget(replaceAllButton);
        buttonLayout->addWidget(closeButton);

        statusLabel = new QLabel();
        statusLabel->setStyleSheet("color: #666666;");

        mainLayout->addLayout(searchLayout);
        mainLayout->addLayout(replaceLayout);
        mainLayout->addLayout(optionsLayout);
        mainLayout->addLayout(buttonLayout);
        mainLayout->addWidget(statusLabel);

        setLayout(mainLayout);
    }

    void setupSignals() {
        connect(searchEdit, &QLineEdit::textChanged, this, &FindDialog::updateUI);
        connect(findNextButton, &QPushButton::clicked, this, &FindDialog::findNext);
        connect(findPrevButton, &QPushButton::clicked, this, &FindDialog::findPrevious);
        connect(replaceButton, &QPushButton::clicked, this, &FindDialog::replace);
        connect(replaceAllButton, &QPushButton::clicked, this, &FindDialog::replaceAll);
        connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    }

private:
    CodeEditor *codeEditor;

    QLineEdit *searchEdit;
    QLineEdit *replaceEdit;
    QCheckBox *caseCheckBox;
    QCheckBox *wholeWordCheckBox;
    QCheckBox *regexCheckBox;
    QCheckBox *backwardCheckBox;

    QPushButton *findNextButton;
    QPushButton *findPrevButton;
    QPushButton *replaceButton;
    QPushButton *replaceAllButton;
    QPushButton *closeButton;

    QLabel *statusLabel;

    int currentPosition;
};

class ReplaceDialog : public FindDialog {
    Q_OBJECT

public:
    explicit ReplaceDialog(CodeEditor *editor, QWidget *parent = nullptr)
        : FindDialog(editor, parent) {
        setWindowTitle("Заменить");
    }
};

class ColorSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ColorSettingsDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setupUI();
        loadColors();
    }

    QColor getAccentColor() const { return accentColor; }
    QColor getBackgroundColor() const { return backgroundColor; }
    QColor getTextColor() const { return textColor; }
    QColor getLineNumberColor() const { return lineNumberColor; }
    QColor getKeywordColor() const { return keywordColor; }
    QColor getCommentColor() const { return commentColor; }
    QColor getStringColor() const { return stringColor; }

signals:
    void colorsChanged(const QColor& accent, const QColor& background, const QColor& text,
                       const QColor& lineNumbers, const QColor& keyword, const QColor& comment,
                       const QColor& string);

private slots:
    void chooseAccentColor() {
        QColor color = QColorDialog::getColor(accentColor, this, "Выберите акцентный цвет");
        if (color.isValid()) {
            accentColor = color;
            updateColorButton(accentButton, color);
        }
    }

    void chooseBackgroundColor() {
        QColor color = QColorDialog::getColor(backgroundColor, this, "Выберите цвет фона");
        if (color.isValid()) {
            backgroundColor = color;
            updateColorButton(backgroundButton, color);
        }
    }

    void chooseTextColor() {
        QColor color = QColorDialog::getColor(textColor, this, "Выберите цвет текста");
        if (color.isValid()) {
            textColor = color;
            updateColorButton(textButton, color);
        }
    }

    void chooseLineNumberColor() {
        QColor color = QColorDialog::getColor(lineNumberColor, this, "Выберите цвет номеров строк");
        if (color.isValid()) {
            lineNumberColor = color;
            updateColorButton(lineNumberButton, color);
        }
    }

    void chooseKeywordColor() {
        QColor color = QColorDialog::getColor(keywordColor, this, "Выберите цвет ключевых слов");
        if (color.isValid()) {
            keywordColor = color;
            updateColorButton(keywordButton, color);
        }
    }

    void chooseCommentColor() {
        QColor color = QColorDialog::getColor(commentColor, this, "Выберите цвет комментариев");
        if (color.isValid()) {
            commentColor = color;
            updateColorButton(commentButton, color);
        }
    }

    void chooseStringColor() {
        QColor color = QColorDialog::getColor(stringColor, this, "Выберите цвет строк");
        if (color.isValid()) {
            stringColor = color;
            updateColorButton(stringButton, color);
        }
    }

    void applyColors() {
        emit colorsChanged(accentColor, backgroundColor, textColor, lineNumberColor,
                           keywordColor, commentColor, stringColor);
        accept();
    }

    void resetToDefault() {
        accentColor = QColor(0, 122, 204);
        backgroundColor = QColor(30, 30, 30);
        textColor = QColor(212, 212, 212);
        lineNumberColor = QColor(133, 133, 133);
        keywordColor = QColor(86, 156, 214);
        commentColor = QColor(106, 153, 85);
        stringColor = QColor(206, 145, 120);

        updateAllColorButtons();
        statusLabel->setText("Цвета сброшены к значениям по умолчанию");
    }

private:
    void setupUI() {
        setWindowTitle("Настройки цветов");
        setFixedSize(400, 500);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        accentButton = createColorButton("Акцентный цвет (статус-бар, выделение):");
        connect(accentButton, &QPushButton::clicked, this, &ColorSettingsDialog::chooseAccentColor);

        backgroundButton = createColorButton("Цвет фона редактора:");
        connect(backgroundButton, &QPushButton::clicked, this, &ColorSettingsDialog::chooseBackgroundColor);

        textButton = createColorButton("Цвет основного текста:");
        connect(textButton, &QPushButton::clicked, this, &ColorSettingsDialog::chooseTextColor);

        lineNumberButton = createColorButton("Цвет номеров строк:");
        connect(lineNumberButton, &QPushButton::clicked, this, &ColorSettingsDialog::chooseLineNumberColor);

        keywordButton = createColorButton("Цвет ключевых слов:");
        connect(keywordButton, &QPushButton::clicked, this, &ColorSettingsDialog::chooseKeywordColor);

        commentButton = createColorButton("Цвет комментариев:");
        connect(commentButton, &QPushButton::clicked, this, &ColorSettingsDialog::chooseCommentColor);

        stringButton = createColorButton("Цвет строк:");
        connect(stringButton, &QPushButton::clicked, this, &ColorSettingsDialog::chooseStringColor);

        statusLabel = new QLabel();
        statusLabel->setStyleSheet("color: #666666;");

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        QPushButton *applyButton = new QPushButton("Применить");
        QPushButton *resetButton = new QPushButton("Сбросить");
        QPushButton *cancelButton = new QPushButton("Отмена");

        buttonLayout->addWidget(applyButton);
        buttonLayout->addWidget(resetButton);
        buttonLayout->addWidget(cancelButton);

        mainLayout->addWidget(accentButton);
        mainLayout->addWidget(backgroundButton);
        mainLayout->addWidget(textButton);
        mainLayout->addWidget(lineNumberButton);
        mainLayout->addWidget(keywordButton);
        mainLayout->addWidget(commentButton);
        mainLayout->addWidget(stringButton);
        mainLayout->addStretch();
        mainLayout->addWidget(statusLabel);
        mainLayout->addLayout(buttonLayout);

        setLayout(mainLayout);

        connect(applyButton, &QPushButton::clicked, this, &ColorSettingsDialog::applyColors);
        connect(resetButton, &QPushButton::clicked, this, &ColorSettingsDialog::resetToDefault);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    }

    QPushButton* createColorButton(const QString& text) {
        QPushButton *button = new QPushButton(text);
        button->setStyleSheet(
            "QPushButton {"
            "    text-align: left;"
            "    padding: 10px;"
            "    background-color: #2D2D30;"
            "    color: #D4D4D4;"
            "    border: 1px solid #3E3E42;"
            "    border-radius: 4px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #404040;"
            "}"
            );
        return button;
    }

    void updateColorButton(QPushButton* button, const QColor& color) {
        QString style = QString(
                            "QPushButton {"
                            "    text-align: left;"
                            "    padding: 10px;"
                            "    background-color: %1;"
                            "    color: %2;"
                            "    border: 2px solid #3E3E42;"
                            "    border-radius: 4px;"
                            "    font-weight: bold;"
                            "}"
                            "QPushButton:hover {"
                            "    background-color: %1;"
                            "    border: 2px solid #505050;"
                            "}"
                            ).arg(color.name()).arg(getContrastColor(color).name());
        button->setStyleSheet(style);
        button->setText(button->text().split(":").first() + ": " + color.name());
    }

    void updateAllColorButtons() {
        updateColorButton(accentButton, accentColor);
        updateColorButton(backgroundButton, backgroundColor);
        updateColorButton(textButton, textColor);
        updateColorButton(lineNumberButton, lineNumberColor);
        updateColorButton(keywordButton, keywordColor);
        updateColorButton(commentButton, commentColor);
        updateColorButton(stringButton, stringColor);
    }

    QColor getContrastColor(const QColor& color) {
        double brightness = (color.red() * 0.299 + color.green() * 0.587 + color.blue() * 0.114);
        return brightness > 128 ? Qt::black : Qt::white;
    }

    void loadColors() {
        QSettings settings("kahendrik", "PawniX");
        accentColor = QColor(settings.value("accentColor", "#007acc").toString());
        backgroundColor = QColor(settings.value("backgroundColor", "#1E1E1E").toString());
        textColor = QColor(settings.value("textColor", "#D4D4D4").toString());
        lineNumberColor = QColor(settings.value("lineNumberColor", "#858585").toString());
        keywordColor = QColor(settings.value("keywordColor", "#569CD6").toString());
        commentColor = QColor(settings.value("commentColor", "#6A9955").toString());
        stringColor = QColor(settings.value("stringColor", "#CE9178").toString());

        updateAllColorButtons();
    }

private:
    QPushButton *accentButton;
    QPushButton *backgroundButton;
    QPushButton *textButton;
    QPushButton *lineNumberButton;
    QPushButton *keywordButton;
    QPushButton *commentButton;
    QPushButton *stringButton;
    QLabel *statusLabel;

    QColor accentColor;
    QColor backgroundColor;
    QColor textColor;
    QColor lineNumberColor;
    QColor keywordColor;
    QColor commentColor;
    QColor stringColor;
};

class PawnEditor : public QMainWindow {
    Q_OBJECT

public:
    PawnEditor(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("PawniX");
        setWindowIcon(QIcon(":/icons/icon_round.png"));

        settings = new QSettings("kahendrik", "PawniX", this);

        loadSettings();

        initUI();

        setupAutoSave();
        setupConnections();

        statusBar()->showMessage("Готово", 3000);

        if (settings->value("showStartPage", true).toBool() && editorTab->count() == 0) {
            showStartPage();
        }
    }

    ~PawnEditor() {
        saveSettings();
    }

public slots:
    void loadFile(const QString& fileName) {
        QElapsedTimer timer;
        timer.start();

        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, "Ошибка", "Не могу открыть файл: " + file.errorString());
            return;
        }

        QByteArray data = file.readAll();
        file.close();

        QString content;
        QString detectedEncoding = "UTF-8";

        QTextCodec* codec = QTextCodec::codecForName("UTF-8");
        QTextCodec* cp1251Codec = QTextCodec::codecForName("Windows-1251");
        QTextCodec* localeCodec = QTextCodec::codecForLocale();

        bool isUtf8 = true;
        QTextCodec::ConverterState state;
        codec->toUnicode(data.constData(), data.size(), &state);
        if (state.invalidChars > 0) {
            isUtf8 = false;
        }

        if (isUtf8) {
            content = codec->toUnicode(data);
            detectedEncoding = "UTF-8";
        } else if (cp1251Codec) {
            content = cp1251Codec->toUnicode(data);
            detectedEncoding = "Windows-1251";
        } else {
            content = localeCodec->toUnicode(data);
            detectedEncoding = "System Locale";
        }

        currentFileEncoding = detectedEncoding;

        CodeEditor* editor = new CodeEditor();

        editor->setUpdatesEnabled(false);

        editor->setEditorFont(editorFont);

        editor->setPlainText(content);

        applyColorsToEditor(editor);

        editor->setUpdatesEnabled(true);

        int index = editorTab->addTab(editor, QFileInfo(fileName).fileName());
        editorTab->setCurrentIndex(index);
        currentFilePath = fileName;

        connect(editor, &CodeEditor::cursorPositionChanged, this, &PawnEditor::updateCursorPosition);
        connect(editor->document(), &QTextDocument::contentsChanged, this, &PawnEditor::documentModified);

        stackedWidget->setCurrentIndex(1);

        updateRecentFiles(fileName);
        updateWindowTitle();
        updateEncodingLabel();

        qint64 elapsed = timer.elapsed();
        statusBar()->showMessage(QString("Файл загружен за %1 мс").arg(elapsed), 3000);
    }

private:
    void initUI() {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(45, 45, 48));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);

        darkPalette.setColor(QPalette::Disabled, QPalette::Text, Qt::darkGray);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, Qt::darkGray);

        qApp->setPalette(darkPalette);

        stackedWidget = new QStackedWidget(this);
        setCentralWidget(stackedWidget);

        startPage = new StartPage(this);
        stackedWidget->addWidget(startPage);

        editorTab = new QTabWidget(this);
        editorTab->setTabsClosable(true);
        editorTab->setMovable(true);
        editorTab->setDocumentMode(true);
        stackedWidget->addWidget(editorTab);

        createMenus();
        createStatusBar();
        createDockWidgets();

        applyColorTheme();
    }

    void createMenus() {
        fileMenu = menuBar()->addMenu("&Файл");
        QAction *newAct = new QAction(QIcon(":/icons/new.png"), "&Новый", this);
        newAct->setShortcut(QKeySequence::New);
        connect(newAct, &QAction::triggered, this, &PawnEditor::newFile);
        fileMenu->addAction(newAct);

        QAction *openAct = new QAction(QIcon(":/icons/open.png"), "&Открыть...", this);
        openAct->setShortcut(QKeySequence::Open);
        connect(openAct, &QAction::triggered, this, &PawnEditor::openFile);
        fileMenu->addAction(openAct);

        QAction *openFolderAct = new QAction(QIcon(":/icons/folder.png"), "Открыть &папку...", this);
        openFolderAct->setShortcut(QKeySequence("Ctrl+Shift+O"));
        connect(openFolderAct, &QAction::triggered, this, &PawnEditor::openFolder);
        fileMenu->addAction(openFolderAct);

        fileMenu->addSeparator();

        QAction *saveAct = new QAction(QIcon(":/icons/save.png"), "&Сохранить", this);
        saveAct->setShortcut(QKeySequence::Save);
        connect(saveAct, &QAction::triggered, this, &PawnEditor::saveFile);
        fileMenu->addAction(saveAct);

        QAction *saveAsAct = new QAction(QIcon(":/icons/save_as.png"), "Сохранить &как...", this);
        saveAsAct->setShortcut(QKeySequence::SaveAs);
        connect(saveAsAct, &QAction::triggered, this, &PawnEditor::saveFileAs);
        fileMenu->addAction(saveAsAct);

        QAction *saveAllAct = new QAction(QIcon(":/icons/save.png"), "Сохранить все", this);
        connect(saveAllAct, &QAction::triggered, this, &PawnEditor::saveAll);
        fileMenu->addAction(saveAllAct);

        fileMenu->addSeparator();
        QAction *exitAct = new QAction("&Выход", this);
        exitAct->setShortcut(QKeySequence::Quit);
        connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
        fileMenu->addAction(exitAct);

        recentMenu = fileMenu->addMenu("&Недавние файлы");
        updateRecentMenu();

        editMenu = menuBar()->addMenu("&Правка");
        QAction *undoAct = new QAction(QIcon(":/icons/undo.png"), "&Отменить", this);
        undoAct->setShortcut(QKeySequence::Undo);
        connect(undoAct, &QAction::triggered, this, &PawnEditor::undo);
        editMenu->addAction(undoAct);

        QAction *redoAct = new QAction(QIcon(":/icons/redo.png"), "&Повторить", this);
        redoAct->setShortcut(QKeySequence::Redo);
        connect(redoAct, &QAction::triggered, this, &PawnEditor::redo);
        editMenu->addAction(redoAct);

        editMenu->addSeparator();

        QAction *cutAct = new QAction(QIcon(":/icons/cut.png"), "&Вырезать", this);
        cutAct->setShortcut(QKeySequence::Cut);
        connect(cutAct, &QAction::triggered, this, &PawnEditor::cut);
        editMenu->addAction(cutAct);

        QAction *copyAct = new QAction(QIcon(":/icons/copy.png"), "Копи&ровать", this);
        copyAct->setShortcut(QKeySequence::Copy);
        connect(copyAct, &QAction::triggered, this, &PawnEditor::copy);
        editMenu->addAction(copyAct);

        QAction *pasteAct = new QAction(QIcon(":/icons/paste.png"), "&Вставить", this);
        pasteAct->setShortcut(QKeySequence::Paste);
        connect(pasteAct, &QAction::triggered, this, &PawnEditor::paste);
        editMenu->addAction(pasteAct);

        QAction *deleteAct = new QAction(QIcon(":/icons/delete.png"), "&Удалить", this);
        deleteAct->setShortcut(QKeySequence::Delete);
        connect(deleteAct, &QAction::triggered, this, &PawnEditor::deleteText);
        editMenu->addAction(deleteAct);

        editMenu->addSeparator();

        QAction *selectAllAct = new QAction(QIcon(":/icons/select_all.png"), "&Выбрать всё", this);
        selectAllAct->setShortcut(QKeySequence::SelectAll);
        connect(selectAllAct, &QAction::triggered, this, &PawnEditor::selectAll);
        editMenu->addAction(selectAllAct);

        editMenu->addSeparator();

        QAction *findAct = new QAction(QIcon(":/icons/search.png"), "&Поиск...", this);
        findAct->setShortcut(QKeySequence::Find);
        connect(findAct, &QAction::triggered, this, &PawnEditor::find);
        editMenu->addAction(findAct);

        QAction *replaceAct = new QAction(QIcon(":/icons/replace.png"), "&Заменить...", this);
        replaceAct->setShortcut(QKeySequence::Replace);
        connect(replaceAct, &QAction::triggered, this, &PawnEditor::replace);
        editMenu->addAction(replaceAct);

        QAction *gotoLineAct = new QAction(QIcon(":/icons/goto.png"), "Перейти к строке...", this);
        gotoLineAct->setShortcut(QKeySequence("Ctrl+G"));
        connect(gotoLineAct, &QAction::triggered, this, &PawnEditor::goToLine);
        editMenu->addAction(gotoLineAct);

        viewMenu = menuBar()->addMenu("&Вид");
        viewMenu->addAction("Стартовая страница", QKeySequence("Ctrl+1"), this, &PawnEditor::showStartPage);
        viewMenu->addSeparator();

        viewMenu->addAction("Увеличить масштаб", QKeySequence::ZoomIn, this, &PawnEditor::zoomIn);
        viewMenu->addAction("Уменьшить масштаб", QKeySequence::ZoomOut, this, &PawnEditor::zoomOut);
        viewMenu->addAction("Сбросить масштаб", QKeySequence("Ctrl+0"), this, &PawnEditor::zoomReset);
        viewMenu->addSeparator();

        viewMenu->addAction("Шрифт редактора...", this, &PawnEditor::selectFont);
        viewMenu->addAction("Сбросить шрифт", this, &PawnEditor::resetFont);
        viewMenu->addSeparator();

        QMenu* colorMenu = viewMenu->addMenu("Настройка цветов");
        colorMenu->addAction("Изменить цветовую палитру...", this, &PawnEditor::openColorSettings);
        colorMenu->addAction("Сбросить цвета к умолчанию", this, &PawnEditor::resetColors);

        buildMenu = menuBar()->addMenu("&Сборка");
        QAction *compileAct = new QAction(QIcon(":/icons/compile.png"), "&Компилировать", this);
        compileAct->setShortcut(QKeySequence("F5"));
        connect(compileAct, &QAction::triggered, this, &PawnEditor::compile);
        buildMenu->addAction(compileAct);
        buildMenu->addSeparator();
        QAction *selectCompilerAct = new QAction("Выбрать компилятор...", this);
        connect(selectCompilerAct, &QAction::triggered, this, &PawnEditor::selectCompiler);
        buildMenu->addAction(selectCompilerAct);

        helpMenu = menuBar()->addMenu("&Справка");
        helpMenu->addAction("&Документация", QKeySequence(" F1"), this, &PawnEditor::openDocumentation);
        helpMenu->addAction("&О программе", this, &PawnEditor::about);
    }

    void createStatusBar() {
        cursorPositionLabel = new QLabel("Строка: 1, Столбец: 1");
        cursorPositionLabel->setStyleSheet("color: white; padding: 0 10px;");
        statusBar()->addPermanentWidget(cursorPositionLabel);

        encodingLabel = new QLabel("UTF-8");
        encodingLabel->setStyleSheet("color: white; padding: 0 10px;");
        statusBar()->addPermanentWidget(encodingLabel);

        modifiedLabel = new QLabel("");
        modifiedLabel->setStyleSheet("color: white; padding: 0 10px;");
        statusBar()->addPermanentWidget(modifiedLabel);
    }

    void createDockWidgets() {
        QDockWidget* fileDock = new QDockWidget("Файловый менеджер", this);
        fileDock->setMinimumWidth(200);
        fileDock->setMaximumWidth(600);

        fileTree = new QTreeView(fileDock);
        fileSystemModel = new QFileSystemModel(this);
        fileSystemModel->setRootPath(QDir::homePath());
        fileTree->setModel(fileSystemModel);
        fileTree->setRootIndex(fileSystemModel->index(QDir::homePath()));
        fileTree->hideColumn(1);
        fileTree->hideColumn(2);
        fileTree->hideColumn(3);

        fileDock->setWidget(fileTree);
        addDockWidget(Qt::LeftDockWidgetArea, fileDock);

        QDockWidget* consoleDock = new QDockWidget("Консоль", this);

        console = new QTextEdit(consoleDock);
        console->setReadOnly(true);
        console->setFont(consoleFont);

        consoleDock->setWidget(console);
        addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    }

    void setupAutoSave() {
        autoSaveTimer = new QTimer(this);
        connect(autoSaveTimer, &QTimer::timeout, this, &PawnEditor::autoSave);

        int interval = settings->value("autoSaveInterval", 300000).toInt();
        if (interval > 0) {
            autoSaveTimer->start(interval);
        }
    }

    void setupConnections() {
        connect(editorTab, &QTabWidget::tabCloseRequested, this, &PawnEditor::closeTab);
        connect(editorTab, &QTabWidget::currentChanged, this, &PawnEditor::updateWindowTitle);
        connect(editorTab, &QTabWidget::currentChanged, this, &PawnEditor::updateEncodingLabel);

        connect(startPage, &StartPage::createNewFile, this, &PawnEditor::newFile);
        connect(startPage, &StartPage::openFile, this, &PawnEditor::openFile);
        connect(startPage, &StartPage::openFolder, this, &PawnEditor::openFolder);
        connect(startPage, &StartPage::openRecentFile, this, &PawnEditor::loadFile);

        connect(fileTree, &QTreeView::doubleClicked, this, &PawnEditor::openFileFromTree);
    }

    void applyColorTheme() {
        QString statusBarStyle = QString(
                                     "QStatusBar {"
                                     "    background-color: %1;"
                                     "    color: white;"
                                     "}"
                                     ).arg(accentColor.name());
        statusBar()->setStyleSheet(statusBarStyle);

        for (int i = 0; i < editorTab->count(); ++i) {
            CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->widget(i));
            if (editor) {
                applyColorsToEditor(editor);
            }
        }
    }

    void applyFontToAllEditors() {
        for (int i = 0; i < editorTab->count(); ++i) {
            CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->widget(i));
            if (editor) {
                editor->setEditorFont(editorFont);
            }
        }
        console->setFont(consoleFont);
    }

    void applyColorsToEditor(CodeEditor* editor) {
        if (!editor) return;

        editor->updateColors(backgroundColor, textColor, accentColor, Qt::white, lineNumberColor);

        PawnHighlighter* highlighter = editor->getHighlighter();
        if (highlighter) {
            highlighter->setThemeColors(keywordColor,
                                        QColor(78, 201, 176),
                                        QColor(197, 134, 192),
                                        commentColor,
                                        stringColor,
                                        QColor(181, 206, 168),
                                        QColor(79, 193, 255),
                                        QColor(212, 212, 212));
        }
    }

private slots:
    void newFile() {
        CodeEditor* editor = new CodeEditor();
        int index = editorTab->addTab(editor, "Новый файл");
        editorTab->setCurrentIndex(index);
        currentFilePath.clear();
        currentFileEncoding = "UTF-8";
        stackedWidget->setCurrentIndex(1);

        editor->setEditorFont(editorFont);

        connect(editor, &CodeEditor::cursorPositionChanged, this, &PawnEditor::updateCursorPosition);
        connect(editor->document(), &QTextDocument::contentsChanged, this, &PawnEditor::documentModified);

        applyColorsToEditor(editor);

        updateCursorPosition();
        updateWindowTitle();
        updateEncodingLabel();
    }

    void openFile() {
        if (maybeSave()) {
            QString fileName = QFileDialog::getOpenFileName(this, "Открыть файл", "",
                                                            "Pawn файлы (*.pwn *.inc);;Текстовые файлы (*.txt);;Все файлы (*.*)");
            if (!fileName.isEmpty()) {
                loadFile(fileName);
            }
        }
    }

    void openFolder() {
        QString folderPath = QFileDialog::getExistingDirectory(this, "Открыть папку", "");
        if (!folderPath.isEmpty()) {
            fileSystemModel->setRootPath(folderPath);
            fileTree->setRootIndex(fileSystemModel->index(folderPath));
        }
    }

    void openFileFromTree(const QModelIndex &index) {
        QString filePath = fileSystemModel->filePath(index);
        QFileInfo fileInfo(filePath);

        if (fileInfo.isFile() && (fileInfo.suffix().toLower() == "pwn" ||
                                  fileInfo.suffix().toLower() == "inc" ||
                                  fileInfo.suffix().toLower() == "cfg" ||
                                  fileInfo.suffix().toLower() == "txt" ||
                                  fileInfo.suffix().toLower() == "ini" ||
                                  fileInfo.suffix().toLower() == "pawn")) {
            loadFile(filePath);
        }
    }

    bool saveFile() {
        CodeEditor* editor = currentEditor();
        if (!editor) return false;

        if (currentFilePath.isEmpty()) {
            return saveFileAs();
        } else {
            return writeFile(currentFilePath, editor->toPlainText(), currentFileEncoding);
        }
    }

    bool saveFileAs() {
        CodeEditor* editor = currentEditor();
        if (!editor) return false;

        QString fileName = QFileDialog::getSaveFileName(this, "Сохранить файл", "",
                                                        "Pawn файлы (*.pwn);;Include файлы (*.inc);;Текстовые файлы (*.txt);;Файлы конфига (*.cfg);;Все файлы (*.*)");
        if (!fileName.isEmpty()) {
            QStringList codecs = {"UTF-8", "Windows-1251"};
            QString codec = QInputDialog::getItem(this, "Выбор кодировки",
                                                  "Выберите кодировку для сохранения:",
                                                  codecs, 0, false);

            if (codec.isEmpty()) {
                codec = "UTF-8";
            }

            if (writeFile(fileName, editor->toPlainText(), codec)) {
                currentFilePath = fileName;
                currentFileEncoding = codec;
                editor->document()->setModified(false);
                updateRecentFiles(fileName);
                updateWindowTitle();
                updateEncodingLabel();
                statusBar()->showMessage("Файл сохранен: " + fileName, 3000);
                return true;
            }
        }
        return false;
    }

    void saveAll() {
        for (int i = 0; i < editorTab->count(); ++i) {
            CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->widget(i));
            if (editor && editor->document()->isModified()) {
                saveFile();
            }
        }
    }

    void selectFont() {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, editorFont, this, "Выберите шрифт редактора");
        if (ok) {
            editorFont = font;
            consoleFont = QFont(editorFont.family(), 10);
            applyFontToAllEditors();
            settings->setValue("editorFont", editorFont);
            statusBar()->showMessage("Шрифт изменен", 3000);
        }
    }

    void resetFont() {
        int ret = QMessageBox::question(this, "Сброс шрифта",
                                        "Вы уверены, что хотите сбросить шрифт к значениям по умолчанию?",
                                        QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            editorFont = QFont("Consolas", 12);
            consoleFont = QFont("Consolas", 10);
            applyFontToAllEditors();
            settings->remove("editorFont");
            statusBar()->showMessage("Шрифт сброшен к значениям по умолчанию", 3000);
        }
    }

    void undo() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->undo();
    }

    void redo() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->redo();
    }

    void cut() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->cut();
    }

    void copy() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->copy();
    }

    void paste() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->paste();
    }

    void deleteText() {
        CodeEditor* editor = currentEditor();
        if (editor) {
            QTextCursor cursor = editor->textCursor();
            if (cursor.hasSelection()) {
                cursor.removeSelectedText();
            } else {
                cursor.deleteChar();
            }
        }
    }

    void selectAll() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->selectAll();
    }

    void find() {
        CodeEditor* editor = currentEditor();
        if (editor) {
            FindDialog dialog(editor, this);
            dialog.exec();
        }
    }

    void replace() {
        CodeEditor* editor = currentEditor();
        if (editor) {
            ReplaceDialog dialog(editor, this);
            dialog.exec();
        }
    }

    void goToLine() {
        CodeEditor* editor = currentEditor();
        if (!editor) return;

        bool ok;
        int line = QInputDialog::getInt(this, "Перейти к строке",
                                        "Введите номер строки:", 1, 1, editor->document()->blockCount(), 1, &ok);
        if (ok) {
            editor->goToLine(line);
        }
    }

    void zoomIn() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->zoomIn(1);
    }

    void zoomOut() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->zoomOut(1);
    }

    void zoomReset() {
        CodeEditor* editor = currentEditor();
        if (editor) editor->zoomReset();
    }

    void compile() {
        if (pawnccPath.isEmpty() || !QFile::exists(pawnccPath)) {
            QMessageBox::warning(this, "Ошибка", "Путь к компилятору не указан или неверен.");
            selectCompiler();
            return;
        }

        CodeEditor* editor = currentEditor();
        if (!editor) return;

        if (currentFilePath.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Сохраните файл перед компиляцией.");
            saveFileAs();
            if (currentFilePath.isEmpty()) return;
        }

        if (editor->document()->isModified()) {
            saveFile();
        }

        console->clear();
        console->append("<b>Начало компиляции...</b>");

        QProcess* process = new QProcess(this);
        QString baseDir = QFileInfo(currentFilePath).absolutePath();
        process->setWorkingDirectory(baseDir);

        QStringList args;
        args << currentFilePath;
        args << "-o" + QFileInfo(currentFilePath).baseName() + ".amx";

        connect(process, &QProcess::readyReadStandardOutput, [this, process]() {
            QString output = QString::fromLocal8Bit(process->readAllStandardOutput());
            console->append(output);
        });

        connect(process, &QProcess::readyReadStandardError, [this, process]() {
            QString error = QString::fromLocal8Bit(process->readAllStandardError());
            console->append("<font color='red'>" + error + "</font>");
        });

        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
                    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                        console->append("<font color='green'><b>Компиляция успешно завершена!</b></font>");
                        statusBar()->showMessage("Компиляция успешно завершена", 5000);
                    } else {
                        console->append("<font color='red'><b>Ошибка компиляции!</b></font>");
                        statusBar()->showMessage("Ошибка компиляции", 5000);
                    }
                    process->deleteLater();
                });

        process->start(pawnccPath, args);
    }

    void compileAndRun() {
        compile();
        console->append("<i>Запуск скомпилированного файла...</i>");
    }

    void selectCompiler() {
        QString path = QFileDialog::getOpenFileName(this, "Выберите компилятор pawncc",
                                                    "", "Pawn Compiler (pawncc.exe)");
        if (!path.isEmpty()) {
            pawnccPath = path;
            settings->setValue("compilerPath", pawnccPath);
            statusBar()->showMessage("Компилятор выбран: " + pawnccPath, 3000);
        }
    }

    void showStartPage() {
        stackedWidget->setCurrentIndex(0);
        startPage->updateRecentFiles(recentFiles);

        QTimer::singleShot(100, [this]() {
            startPage->startTitleAnimation();
        });
    }

    void openColorSettings() {
        ColorSettingsDialog dialog(this);
        connect(&dialog, &ColorSettingsDialog::colorsChanged, this, [this](
                                                                        const QColor& accent, const QColor& background, const QColor& text,
                                                                        const QColor& lineNumbers, const QColor& keyword, const QColor& comment,
                                                                        const QColor& string) {

            accentColor = accent;
            backgroundColor = background;
            textColor = text;
            lineNumberColor = lineNumbers;
            keywordColor = keyword;
            commentColor = comment;
            stringColor = string;

            settings->setValue("accentColor", accent.name());
            settings->setValue("backgroundColor", background.name());
            settings->setValue("textColor", text.name());
            settings->setValue("lineNumberColor", lineNumbers.name());
            settings->setValue("keywordColor", keyword.name());
            settings->setValue("commentColor", comment.name());
            settings->setValue("stringColor", string.name());

            applyColorTheme();

            statusBar()->showMessage("Цветовая палитра обновлена", 3000);
        });

        dialog.exec();
    }

    void resetColors() {
        int ret = QMessageBox::question(this, "Сброс цветов",
                                        "Вы уверены, что хотите сбросить цвета к значениям по умолчанию?",
                                        QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            accentColor = QColor(0, 122, 204);
            backgroundColor = QColor(30, 30, 30);
            textColor = QColor(212, 212, 212);
            lineNumberColor = QColor(133, 133, 133);
            keywordColor = QColor(86, 156, 214);
            commentColor = QColor(106, 153, 85);
            stringColor = QColor(206, 145, 120);

            settings->remove("accentColor");
            settings->remove("backgroundColor");
            settings->remove("textColor");
            settings->remove("lineNumberColor");
            settings->remove("keywordColor");
            settings->remove("commentColor");
            settings->remove("stringColor");

            applyColorTheme();

            statusBar()->showMessage("Цвета сброшены к значениям по умолчанию", 3000);
        }
    }

    void openDocumentation() {
        //QDesktopServices::openUrl(QUrl("https://www.compuphase.com/pawn/pawn.htm"));
    }

    void about() {
        QString aboutText =
            "<center><h2>PawniX</h2></center>"
            "<p><b><center>Версия 2.0</center></b></p>"
            "<p>Современная среда разработки для языка Pawn</p>"
            "<hr>"
            "<p><b>Основные возможности:</b></p>"
            "<ul>"
            "<li>Продвинутый редактор кода с подсветкой синтаксиса</li>"
            "<li>Настраиваемая цветовая палитра</li>"
            "<li>Настраиваемый шрифт редактора</li>"
            "<li>Нумерация строк и подсветка текущей строки</li>"
            "<li>Система закладок</li>"
            "<li>Поиск и замена с поддержкой регулярных выражений</li>"
            "<li>Интегрированный компилятор Pawn</li>"
            "<li>Консоль вывода</li>"
            "<li>Файловый менеджер</li>"
            "<li>Автосохранение и резервное копирование</li>"
            "</ul>"
            "<hr>"
            "<p><b>Разработчик:</b> kahendrik</p>"
            "<p><b>GitHub:</b> <a href='https://github.com/kahendrik/pawnix'>https://github.com/kahendrik/pawnix</a></p>"
            "<p><b>Лицензия:</b> MIT License</p>";

        QMessageBox::about(this, "О программе PawniX", aboutText);
    }

    void autoSave() {
        CodeEditor* editor = currentEditor();
        if (!editor || !editor->document()->isModified() || currentFilePath.isEmpty()) {
            return;
        }

        QString backupPath = currentFilePath + ".autosave";
        if (writeFile(backupPath, editor->toPlainText(), currentFileEncoding)) {
            QFileInfo info(backupPath);
            QString msg = QString("Автосохранение: %1 (%2)")
                              .arg(info.fileName())
                              .arg(info.lastModified().toString("hh:mm:ss"));
            statusBar()->showMessage(msg, 2000);
        }
    }

    void updateCursorPosition() {
        CodeEditor* editor = currentEditor();
        if (editor) {
            QTextCursor cursor = editor->textCursor();
            int line = cursor.blockNumber() + 1;
            int column = cursor.positionInBlock() + 1;
            cursorPositionLabel->setText(QString("Строка: %1, Столбец: %2").arg(line).arg(column));
        }
    }

    void documentModified() {
        CodeEditor* editor = currentEditor();
        if (editor) {
            modifiedLabel->setText(editor->document()->isModified() ? "●" : "");
            updateWindowTitle();
        }
    }

    void updateEncodingLabel() {
        encodingLabel->setText(currentFileEncoding);
    }

    void updateRecentFiles(const QString& filePath) {
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);

        if (recentFiles.size() > 10) {
            recentFiles.removeLast();
        }

        settings->setValue("recentFiles", recentFiles);

        if (startPage) {
            startPage->updateRecentFiles(recentFiles);
        }

        updateRecentMenu();
    }

    void updateRecentMenu() {
        recentMenu->clear();

        for (const QString& file : recentFiles) {
            QFileInfo info(file);
            QString text = info.fileName() + " (" + info.absolutePath() + ")";
            QAction* action = recentMenu->addAction(text);
            connect(action, &QAction::triggered, [this, file]() {
                loadFile(file);
            });
        }

        if (!recentFiles.isEmpty()) {
            recentMenu->addSeparator();
            QAction* clearAction = recentMenu->addAction("Очистить список");
            connect(clearAction, &QAction::triggered, [this]() {
                recentFiles.clear();
                settings->setValue("recentFiles", recentFiles);
                updateRecentMenu();
            });
        }
    }

    void updateWindowTitle() {
        CodeEditor* editor = currentEditor();
        if (!editor) {
            setWindowTitle("PawniX");
            return;
        }

        QString title = "PawniX";
        if (!currentFilePath.isEmpty()) {
            title += " - " + QFileInfo(currentFilePath).fileName();
            if (editor->document()->isModified()) {
                title += "*";
            }
        } else {
            title += " - [Новый файл]";
            if (editor->document()->isModified()) {
                title += "*";
            }
        }

        setWindowTitle(title);
    }

    void closeTab(int index) {
        if (maybeSave()) {
            QWidget* widget = editorTab->widget(index);
            editorTab->removeTab(index);
            delete widget;

            if (editorTab->count() == 0) {
                showStartPage();
            }
        }
    }

    bool maybeSave() {
        CodeEditor* editor = currentEditor();
        if (!editor || !editor->document()->isModified()) {
            return true;
        }

        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, "PawniX",
                                   tr("Документ был изменен.\n"
                                      "Сохранить изменения?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (ret == QMessageBox::Save) {
            return saveFile();
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }

        return true;
    }

    bool writeFile(const QString& path, const QString& content, const QString& encoding) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Ошибка",
                                 "Не могу сохранить файл: " + file.errorString());
            return false;
        }

        QByteArray data;
        if (encoding == "UTF-8") {
            data = content.toUtf8();
        } else if (encoding == "Windows-1251") {
            QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
            if (codec) {
                data = codec->fromUnicode(content);
            } else {
                data = content.toLocal8Bit();
            }
        } else {
            data = content.toUtf8();
        }

        file.write(data);
        file.close();

        CodeEditor* editor = currentEditor();
        if (editor && path == currentFilePath) {
            editor->document()->setModified(false);
            updateWindowTitle();
        }

        return true;
    }

    void loadSettings() {
        restoreGeometry(settings->value("geometry").toByteArray());
        restoreState(settings->value("windowState").toByteArray());

        pawnccPath = settings->value("compilerPath", "").toString();
        recentFiles = settings->value("recentFiles").toStringList();
        currentFileEncoding = settings->value("defaultEncoding", "UTF-8").toString();

        editorFont = settings->value("editorFont", QFont("Consolas", 12)).value<QFont>();
        consoleFont = QFont(editorFont.family(), 10);

        accentColor = QColor(settings->value("accentColor", "#007acc").toString());
        backgroundColor = QColor(settings->value("backgroundColor", "#1E1E1E").toString());
        textColor = QColor(settings->value("textColor", "#D4D4D4").toString());
        lineNumberColor = QColor(settings->value("lineNumberColor", "#858585").toString());
        keywordColor = QColor(settings->value("keywordColor", "#569CD6").toString());
        commentColor = QColor(settings->value("commentColor", "#6A9955").toString());
        stringColor = QColor(settings->value("stringColor", "#CE9178").toString());
    }

    void saveSettings() {
        settings->setValue("geometry", saveGeometry());
        settings->setValue("windowState", saveState());
        settings->setValue("compilerPath", pawnccPath);
        settings->setValue("recentFiles", recentFiles);
        settings->setValue("defaultEncoding", currentFileEncoding);
        settings->setValue("editorFont", editorFont);

        settings->setValue("accentColor", accentColor.name());
        settings->setValue("backgroundColor", backgroundColor.name());
        settings->setValue("textColor", textColor.name());
        settings->setValue("lineNumberColor", lineNumberColor.name());
        settings->setValue("keywordColor", keywordColor.name());
        settings->setValue("commentColor", commentColor.name());
        settings->setValue("stringColor", stringColor.name());
    }

    CodeEditor* currentEditor() const {
        return qobject_cast<CodeEditor*>(editorTab->currentWidget());
    }

private:
    QStackedWidget* stackedWidget;
    StartPage* startPage;
    QTabWidget* editorTab;

    QMenu* fileMenu;
    QMenu* recentMenu;
    QMenu* editMenu;
    QMenu* viewMenu;
    QMenu* buildMenu;
    QMenu* helpMenu;

    QLabel* cursorPositionLabel;
    QLabel* encodingLabel;
    QLabel* modifiedLabel;

    QTreeView* fileTree;
    QTextEdit* console;

    QFileSystemModel* fileSystemModel;

    QTimer* autoSaveTimer;

    QSettings* settings;
    QString pawnccPath;
    QString currentFilePath;
    QString currentFileEncoding;
    QStringList recentFiles;

    QFont editorFont;
    QFont consoleFont;

    QColor accentColor;
    QColor backgroundColor;
    QColor textColor;
    QColor lineNumberColor;
    QColor keywordColor;
    QColor commentColor;
    QColor stringColor;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName("PawniX");
    QApplication::setOrganizationName("kahendrik");
    QApplication::setApplicationVersion("2.0");

    QFont font("Segoe UI", 10);
    app.setFont(font);

    PawnEditor editor;
    editor.resize(1000, 800);
    editor.show();

    if (argc > 1) {
        QString filePath = QString::fromLocal8Bit(argv[1]);
        if (QFile::exists(filePath)) {
            QTimer::singleShot(100, [&editor, filePath]() {
                editor.loadFile(filePath);
            });
        }
    }

    return app.exec();
}

#include "main.moc"
