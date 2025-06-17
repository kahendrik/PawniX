#include <includes.h>
class CodeEditor;
class PawnEditor;
class PawnHighlighter : public QSyntaxHighlighter {
public:
    PawnHighlighter(QTextDocument* parent = nullptr) : QSyntaxHighlighter(parent) {
        setupRules();
    }
private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightRule> rules;
    QTextCharFormat multiLineCommentFormat;
    void setupRules() {
        std::vector<std::string> keywords = {
            "assert", "break", "case", "const", "continue", "default", "do", "else", "enum", "for",
            "forward", "functag", "goto", "if", "native", "new", "operator", "public", "return",
            "sizeof", "static", "stock", "switch", "tagof", "while", "defined"
        };
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor("#569CD6"));
        keywordFormat.setFontWeight(QFont::Bold);
        for (const std::string& keyword : keywords) {
            HighlightRule rule;
            rule.pattern = QRegularExpression("\\b" + QString::fromStdString(keyword) + "\\b");
            rule.format = keywordFormat;
            rules.append(rule);
        }
        QTextCharFormat preprocessorFormat;
        preprocessorFormat.setForeground(QColor("#C586C0"));
        HighlightRule preprocessorRule;
        preprocessorRule.pattern = QRegularExpression("#\\s*\\w+");
        preprocessorRule.format = preprocessorFormat;
        rules.append(preprocessorRule);
        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor("#CE9178"));
        HighlightRule stringRule;
        stringRule.pattern = QRegularExpression("\".*?\"");
        stringRule.format = stringFormat;
        rules.append(stringRule);
        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor("#B5CEA8"));
        HighlightRule numberRule;
        numberRule.pattern = QRegularExpression("\\b\\d+\\b");
        numberRule.format = numberFormat;
        rules.append(numberRule);
        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor("#6A9955"));
        HighlightRule commentRule;
        commentRule.pattern = QRegularExpression("//[^\\n]*");
        commentRule.format = commentFormat;
        rules.append(commentRule);
        multiLineCommentFormat = commentFormat;
        multiLineCommentFormat.setForeground(QColor("#6A9955"));
    }
    void highlightBlock(const QString& text) override {
        for (const HighlightRule& rule : rules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
        setCurrentBlockState(0);
        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf("/*");
        }
        while (startIndex >= 0) {
            int endIndex = text.indexOf("*/", startIndex);
            int commentLength;
            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + 2;
            }
            setFormat(startIndex, commentLength, multiLineCommentFormat);
            startIndex = text.indexOf("/*", startIndex + commentLength);
        }
    }
};
class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    CodeEditor(QWidget* parent = nullptr) : QPlainTextEdit(parent) {
        QFont font;
        font.setFamily("Consolas");
        font.setPointSize(12);
        setFont(font);
        QPalette p = palette();
        p.setColor(QPalette::Base, QColor("#1E1E1E"));
        p.setColor(QPalette::Text, QColor("#D4D4D4"));
        p.setColor(QPalette::Highlight, QColor("#264F78"));
        p.setColor(QPalette::HighlightedText, Qt::white);
        setPalette(p);
        new PawnHighlighter(document());
        setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
        connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
        connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
        connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);
        lineNumberArea = new LineNumberArea(this);
        updateLineNumberAreaWidth(0);
        highlightCurrentLine();
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
    void lineNumberAreaPaintEvent(QPaintEvent *event) {
        QPainter painter(lineNumberArea);
        painter.fillRect(event->rect(), QColor("#252526"));
        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = (int)blockBoundingGeometry(block).translated(contentOffset()).top();
        int bottom = top + (int)blockBoundingRect(block).height();
        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(QColor("#7A7A7A"));
                painter.drawText(0, top, lineNumberArea->width() - 5, fontMetrics().height(),
                                 Qt::AlignRight, number);
            }
            block = block.next();
            top = bottom;
            bottom = top + (int)blockBoundingRect(block).height();
            ++blockNumber;
        }
    }
    void findText(const QString& text, bool caseSensitive, bool wholeWords) {
        QTextDocument* doc = document();
        QTextCursor cursor(doc);
        QTextCursor highlightCursor(doc);
        QTextCharFormat fmt;
        fmt.setBackground(Qt::yellow);
        QList<QTextEdit::ExtraSelection> selections;
        setExtraSelections(selections);
        if (text.isEmpty()) return;
        QRegularExpression pattern(text);
        pattern.setPatternOptions(wholeWords ? QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption);
        if (caseSensitive) pattern.setPatternOptions(pattern.patternOptions() ^ QRegularExpression::CaseInsensitiveOption);
        while (!cursor.isNull() && cursor.movePosition(QTextCursor::NextBlock)) {
            QRegularExpressionMatchIterator it = pattern.globalMatch(cursor.block().text());
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                highlightCursor.setPosition(cursor.block().position() + match.capturedStart());
                highlightCursor.setPosition(cursor.block().position() + match.capturedEnd(), QTextCursor::KeepAnchor);
                QTextEdit::ExtraSelection selection;
                selection.cursor = highlightCursor;
                selection.format = fmt;
                selections.append(selection);
            }
        }
        setExtraSelections(selections);
    }
    void replaceText(const QString& searchText, const QString& replaceText, bool caseSensitive, bool wholeWords) {
        QTextCursor cursor(document());
        int count = 0;
        QRegularExpression pattern(searchText);
        pattern.setPatternOptions(wholeWords ? QRegularExpression::UseUnicodePropertiesOption | QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption);
        if (caseSensitive) pattern.setPatternOptions(pattern.patternOptions() ^ QRegularExpression::CaseInsensitiveOption);
        while (!cursor.isNull() && cursor.movePosition(QTextCursor::NextBlock)) {
            QRegularExpressionMatchIterator it = pattern.globalMatch(cursor.block().text());
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                cursor.setPosition(cursor.block().position() + match.capturedStart());
                cursor.setPosition(cursor.block().position() + match.capturedEnd(), QTextCursor::KeepAnchor);
                cursor.insertText(replaceText);
                count++;
            }
        }
        if (count > 0) {
            QTextCursor tc = textCursor();
            tc.movePosition(QTextCursor::Start);
            setTextCursor(tc);
        }
    }
    void goToLine(int lineNumber) {
        if (lineNumber < 1 || lineNumber > blockCount()) return;
        QTextCursor cursor(document()->findBlockByNumber(lineNumber - 1));
        cursor.movePosition(QTextCursor::StartOfLine);
        setTextCursor(cursor);
        verticalScrollBar()->setValue(cursor.position());
    }
protected:
    void resizeEvent(QResizeEvent *event) override {
        QPlainTextEdit::resizeEvent(event);
        QRect cr = contentsRect();
        lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    }
private slots:
    void updateLineNumberAreaWidth(int newBlockCount) {
        Q_UNUSED(newBlockCount);
        setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
    }
    void updateLineNumberArea(const QRect& rect, int dy) {
        if (dy)
            lineNumberArea->scroll(0, dy);
        else
            lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    }
    void highlightCurrentLine() {
        QList<QTextEdit::ExtraSelection> extraSelections;
        if (!isReadOnly()) {
            QTextEdit::ExtraSelection selection;
            selection.format.setBackground(QColor("#2D2D30"));
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            selection.cursor = textCursor();
            selection.cursor.clearSelection();
            extraSelections.append(selection);
        }
        setExtraSelections(extraSelections);
    }
private:
    class LineNumberArea : public QWidget {
    public:
        LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}
    protected:
        void paintEvent(QPaintEvent *event) override {
            codeEditor->lineNumberAreaPaintEvent(event);
        }
    private:
        CodeEditor *codeEditor;
    };
    LineNumberArea *lineNumberArea;
};
class FindDialog : public QDialog {
    Q_OBJECT
public:
    FindDialog(CodeEditor* editor, QWidget* parent = nullptr) : QDialog(parent), codeEditor(editor) {
        setupUI();
        connect(buttonBox, &QDialogButtonBox::accepted, this, &FindDialog::onFind);
    }
    QString searchText() const { return searchEdit->text(); }
    bool caseSensitive() const { return caseCheckBox->isChecked(); }
    bool wholeWords() const { return wordCheckBox->isChecked(); }
protected:
    QDialogButtonBox* buttonBox;
    CodeEditor* codeEditor;
private:
    void setupUI() {
        QVBoxLayout* layout = new QVBoxLayout(this);
        QHBoxLayout* hLayout = new QHBoxLayout();
        searchEdit = new QLineEdit();
        hLayout->addWidget(new QLabel("Найти:"));
        hLayout->addWidget(searchEdit);
        caseCheckBox = new QCheckBox("Регистр");
        wordCheckBox = new QCheckBox("Только слова");
        hLayout->addWidget(caseCheckBox);
        hLayout->addWidget(wordCheckBox);
        buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addLayout(hLayout);
        layout->addWidget(buttonBox);
        setWindowTitle("Поиск");
    }
    void onFind() {
        codeEditor->findText(searchText(), caseSensitive(), wholeWords());
    }
    QLineEdit* searchEdit;
    QCheckBox* caseCheckBox;
    QCheckBox* wordCheckBox;
};
class ReplaceDialog : public FindDialog {
    Q_OBJECT
public:
    ReplaceDialog(CodeEditor* editor, QWidget* parent = nullptr) : FindDialog(editor, parent) {
        setupReplaceUI();
        connect(buttonBox, &QDialogButtonBox::accepted, this, &ReplaceDialog::onReplace);
    }
    QString replaceText() const { return replaceEdit->text(); }
private:
    void setupReplaceUI() {
        QVBoxLayout* layout = static_cast<QVBoxLayout*>(this->layout());
        QHBoxLayout* hLayout = new QHBoxLayout();
        replaceEdit = new QLineEdit();
        hLayout->addWidget(new QLabel("Заменить на:"));
        hLayout->addWidget(replaceEdit);
        QDialogButtonBox* replaceButtons = new QDialogButtonBox();
        QPushButton* replaceButton = new QPushButton("Заменить");
        QPushButton* replaceAllButton = new QPushButton("Заменить все");
        connect(replaceButton, &QPushButton::clicked, this, &ReplaceDialog::onReplace);
        connect(replaceAllButton, &QPushButton::clicked, this, &ReplaceDialog::onReplaceAll);
        replaceButtons->addButton(replaceButton, QDialogButtonBox::ActionRole);
        replaceButtons->addButton(replaceAllButton, QDialogButtonBox::ActionRole);
        layout->insertLayout(1, hLayout);
        layout->addWidget(replaceButtons);
        setWindowTitle("Замена");
    }
    void onReplace() {
        codeEditor->replaceText(searchText(), replaceText(), caseSensitive(), wholeWords());
    }
    void onReplaceAll() {
        codeEditor->replaceText(searchText(), replaceText(), caseSensitive(), wholeWords());
    }
    QLineEdit* replaceEdit;
    QPushButton* replaceAllButton;
};
class StartPage : public QWidget {
    Q_OBJECT
public:
    StartPage(QWidget *parent = nullptr);
    void updateRecentFiles(const QStringList &files);
signals:
    void openRecentFile(const QString &filePath);
    void createNewFile();
    void openFile();
    void openFolder();
private:
    QListWidget *recentList;
};
StartPage::StartPage(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(50, 50, 50, 50);
    QLabel *title = new QLabel("Добро пожаловать в PawniX!");
    QFont titleFont = title->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setStyleSheet("color: #D4D4D4; margin-bottom: 30px;");
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *newButton = new QPushButton(QIcon(":/icons/new.png"), "Новый файл");
    QPushButton *openFileButton = new QPushButton(QIcon(":/icons/open.png"), "Открыть файл");
    QPushButton *openFolderButton = new QPushButton(QIcon(":/icons/folder.png"), "Открыть папку");
    newButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; min-width: 150px; }");
    openFileButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; min-width: 150px; }");
    openFolderButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; min-width: 150px; }");
    buttonLayout->addStretch();
    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(openFileButton);
    buttonLayout->addWidget(openFolderButton);
    buttonLayout->addStretch();
    QLabel *recentLabel = new QLabel("Недавние файлы:");
    recentLabel->setStyleSheet("font-size: 14px; color: #D4D4D4; margin-top: 30px;");
    recentList = new QListWidget();
    recentList->setStyleSheet("QListWidget { background: #252526; color: #D4D4D4; border: 1px solid #3A3A3A; }"
                              "QListWidget::item { padding: 8px; }"
                              "QListWidget::item:hover { background: #2A2D2E; }"
                              "QListWidget::item:selected { background: #37373D; }");
    recentList->setMinimumHeight(200);
    layout->addWidget(title);
    layout->addLayout(buttonLayout);
    layout->addWidget(recentLabel);
    layout->addWidget(recentList);
    connect(newButton, &QPushButton::clicked, this, &StartPage::createNewFile);
    connect(openFileButton, &QPushButton::clicked, this, &StartPage::openFile);
    connect(openFolderButton, &QPushButton::clicked, this, &StartPage::openFolder);
    connect(recentList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
        emit openRecentFile(item->data(Qt::UserRole).toString());
    });
}
void StartPage::updateRecentFiles(const QStringList &files) {
    recentList->clear();
    for (const QString &file : files) {
        QFileInfo fileInfo(file);
        QListWidgetItem *item = new QListWidgetItem(
            QIcon(":/icons/file.png"),
            fileInfo.fileName() + "" + fileInfo.absolutePath()
            );
        item->setData(Qt::UserRole, file);
        recentList->addItem(item);
    }
}
class PawnEditor : public QMainWindow {
    Q_OBJECT
public:
    PawnEditor() {
        setWindowIcon(QIcon(":/icons/pawn_editor.ico"));
        stackedWidget = new QStackedWidget(this);
        setCentralWidget(stackedWidget);
        startPage = new StartPage(this);
        stackedWidget->addWidget(startPage);
        editorTab = new QTabWidget();
        editorTab->setTabsClosable(true);
        stackedWidget->addWidget(editorTab);
        loadSettings();
        if (pawnccPath.isEmpty()) {
            findPawnCompiler(false);
        }
        createMenus();
        createToolBars();
        statusBar()->showMessage("Готово");
        if (!pawnccPath.isEmpty()) {
            statusBar()->addPermanentWidget(new QLabel("Компилятор: " + pawnccPath));
        }
        createDockWidgets();
        setupCompleter();
        connect(editorTab, &QTabWidget::tabCloseRequested, this, &PawnEditor::closeTab);
        connect(editorTab, &QTabWidget::currentChanged, this, &PawnEditor::updateWindowTitle);
        connect(startPage, &StartPage::createNewFile, this, &PawnEditor::newFile);
        connect(startPage, &StartPage::openFile, this, &PawnEditor::open);
        connect(startPage, &StartPage::openFolder, this, &PawnEditor::openFolder);
        connect(startPage, &StartPage::openRecentFile, this, &PawnEditor::loadFile);
        if (currentFile.isEmpty() && currentFolder.isEmpty()) {
            stackedWidget->setCurrentIndex(0);
            startPage->updateRecentFiles(recentFiles);
        } else {
            stackedWidget->setCurrentIndex(1);
        }
    }
    ~PawnEditor() {
        saveSettings();
    }
private:
    QTabWidget* editorTab;
    QStackedWidget* stackedWidget;
    StartPage* startPage;
    QString currentFile;
    QString currentFolder;
    QString pawnccPath;
    QCompleter* completer;
    QFileSystemModel* fsModel;
    QTreeView* fileTree;
    QTextEdit* errorConsole = nullptr;
    QProcess* pawnProcess = nullptr;
    QPushButton* openFolderBtn;
    QStringList recentFiles;
    void createMenus() {
        clearMenus();
        fileMenu = menuBar()->addMenu("&Файл");
        fileMenu->addAction("&Новый", QKeySequence::New, this, &PawnEditor::newFile);
        fileMenu->addAction("&Открыть файл...", QKeySequence::Open, this, &PawnEditor::open);
        fileMenu->addAction("Открыть &папку...", QKeySequence("Ctrl+Shift+O"), this, &PawnEditor::openFolder);
        recentMenu = fileMenu->addMenu("Недавние файлы");
        updateRecentMenu(recentMenu);
        fileMenu->addAction("&Сохранить", QKeySequence::Save, this, &PawnEditor::save);
        fileMenu->addAction("Сохранить &как...", QKeySequence::SaveAs, this, &PawnEditor::saveAs);
        fileMenu->addSeparator();
        fileMenu->addAction("Выбрать &компилятор...", this, &PawnEditor::setCompilerPath);
        fileMenu->addSeparator();
        fileMenu->addAction("&Выход", QKeySequence::Quit, qApp, &QApplication::quit);
        editMenu = menuBar()->addMenu("&Правка");
        editMenu->addAction("&Отменить", QKeySequence::Undo, this, &PawnEditor::onUndo);
        editMenu->addAction("&Повторить", QKeySequence::Redo, this, &PawnEditor::onRedo);
        editMenu->addSeparator();
        editMenu->addAction("&Вырезать", QKeySequence::Cut, this, &PawnEditor::onCut);
        editMenu->addAction("&Копировать", QKeySequence::Copy, this, &PawnEditor::onCopy);
        editMenu->addAction("&Вставить", QKeySequence::Paste, this, &PawnEditor::onPaste);
        editMenu->addSeparator();
        editMenu->addAction("&Поиск...", QKeySequence::Find, this, &PawnEditor::find);
        editMenu->addAction("За&менить...", QKeySequence::Replace, this, &PawnEditor::replace);
        buildMenu = menuBar()->addMenu("&Сборка");
        buildMenu->addAction("&Компилировать", QKeySequence("F5"), this, &PawnEditor::compile);
        helpMenu = menuBar()->addMenu("&Справка");
        helpMenu->addAction("&О программе", this, &PawnEditor::about);
        helpMenu->addAction("&Документация", this, &PawnEditor::openDocumentation);
    }
    void clearMenus() {
        menuBar()->clear();
    }
    void createToolBars() {
        clearToolBars();
        QToolBar* fileToolBar = addToolBar("Файл");
        fileToolBar->addAction(QIcon(":/icons/new.png"), "Новый", this, &PawnEditor::newFile);
        fileToolBar->addAction(QIcon(":/icons/open.png"), "Открыть файл", this, &PawnEditor::open);
        fileToolBar->addAction(QIcon(":/icons/folder.png"), "Открыть папку", this, &PawnEditor::openFolder);
        fileToolBar->addAction(QIcon(":/icons/save.png"), "Сохранить", this, &PawnEditor::save);
        QToolBar* buildToolBar = addToolBar("Сборка");
        buildToolBar->addAction(QIcon(":/icons/compile.png"), "Компилировать", this, &PawnEditor::compile);
        QToolBar* editToolBar = addToolBar("Правка");
        editToolBar->addAction(QIcon(":/icons/undo.png"), "Отменить", this, &PawnEditor::onUndo);
        editToolBar->addAction(QIcon(":/icons/redo.png"), "Повторить", this, &PawnEditor::onRedo);
        editToolBar->addSeparator();
        editToolBar->addAction(QIcon(":/icons/cut.png"), "Вырезать", this, &PawnEditor::onCut);
        editToolBar->addAction(QIcon(":/icons/copy.png"), "Копировать", this, &PawnEditor::onCopy);
        editToolBar->addAction(QIcon(":/icons/paste.png"), "Вставить", this, &PawnEditor::onPaste);
        editToolBar->addAction(QIcon(":/icons/search.png"), "Найти", this, &PawnEditor::find);
        editToolBar->addAction(QIcon(":/icons/replace.png"), "Заменить", this, &PawnEditor::replace);
        editToolBar->addAction(QIcon(":/icons/goto.png"), "Перейти к строке", this, &PawnEditor::goToLine);
    }
    void clearToolBars() {
        QList<QToolBar*> toolBars = findChildren<QToolBar*>();
        for (QToolBar* tb : toolBars) {
            removeToolBar(tb);
            delete tb;
        }
    }
    void createDockWidgets() {
        QDockWidget* fileDock = new QDockWidget("Файловый менеджер", this);
        QWidget* container = new QWidget(fileDock);
        QVBoxLayout* layout = new QVBoxLayout(container);
        openFolderBtn = new QPushButton("Открыть папку");
        openFolderBtn->setStyleSheet("QPushButton { background: #3A3A3A; color: white; padding: 8px; font-weight: bold; }");
        connect(openFolderBtn, &QPushButton::clicked, this, &PawnEditor::openFolder);
        fsModel = new QFileSystemModel(this);
        fsModel->setRootPath("");
        fsModel->setNameFilters(QStringList() << "*.pwn" << "*.inc" << "*.txt" << "*.cfg" << "*.ini");
        fsModel->setNameFilterDisables(false);
        fileTree = new QTreeView();
        fileTree->setModel(fsModel);
        fileTree->setHeaderHidden(true);
        fileTree->setAnimated(true);
        fileTree->setIndentation(15);
        fileTree->setStyleSheet("QTreeView { background: #252526; color: #D4D4D4; }");
        fileTree->setSortingEnabled(true);
        fileTree->sortByColumn(0, Qt::AscendingOrder);
        for (int i = 1; i < fsModel->columnCount(); ++i) {
            fileTree->hideColumn(i);
        }
        fileTree->setVisible(false);
        layout->addWidget(openFolderBtn);
        layout->addWidget(fileTree);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        container->setLayout(layout);
        fileDock->setWidget(container);
        addDockWidget(Qt::LeftDockWidgetArea, fileDock);
        if (!currentFolder.isEmpty()) {
            QDir dir(currentFolder);
            dir.setSorting(QDir::DirsFirst | QDir::Name);
            QModelIndex rootIndex = fsModel->index(dir.path());
            fileTree->setRootIndex(rootIndex);
            fileTree->setVisible(true);
            openFolderBtn->setVisible(false);
        }
        connect(fileTree, &QTreeView::doubleClicked, this, &PawnEditor::loadSelectedFile);
    }
    void setupCompleter() {
        std::vector<std::string> pawnKeywords = {
            "assert", "break", "case", "const", "continue", "default", "do", "else", "enum", "for",
            "forward", "functag", "goto", "if", "native", "new", "operator", "public", "return",
            "sizeof", "static", "stock", "switch", "tagof", "while", "defined"
        };
        QStringList qKeywords;
        for (const auto& kw : pawnKeywords) {
            qKeywords << QString::fromStdString(kw);
        }
        completer = new QCompleter(qKeywords, this);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setWidget(editorTab->currentWidget());
        completer->setCompletionMode(QCompleter::PopupCompletion);
        completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
        completer->popup()->setStyleSheet("background: #252526; color: #D4D4D4; selection-background-color: #264F78;");
        QObject::connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
                         this, &PawnEditor::insertCompletion);
    }
    void insertCompletion(const QString& completion) {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (currentEditor) {
            QTextCursor cursor = currentEditor->textCursor();
            cursor.insertText(completion);
            currentEditor->setTextCursor(cursor);
        }
    }
    void loadSettings() {
        QSettings settings("kahendrik", "PawniX");
        restoreGeometry(settings.value("geometry").toByteArray());
        restoreState(settings.value("windowState").toByteArray());
        pawnccPath = settings.value("compilerPath", "").toString();
        currentFolder = settings.value("currentFolder", "").toString();
        recentFiles = settings.value("recentFiles").toStringList();
    }
    void saveSettings() {
        QSettings settings("kahendrik", "PawniX");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        settings.setValue("compilerPath", pawnccPath);
        settings.setValue("currentFolder", currentFolder);
        settings.setValue("recentFiles", recentFiles);
    }
    void findPawnCompiler(bool showDialog = true) {
        std::vector<std::string> possiblePaths = {
            "pawno/pawncc.exe",
            "../pawno/pawncc.exe",
            (QApplication::applicationDirPath() + "/pawno/pawncc.exe").toStdString()
        };
        bool found = false;
        for (const auto &path : possiblePaths) {
            QString qPath = QString::fromStdString(path);
            if (QFile::exists(qPath)) {
                pawnccPath = qPath;
                found = true;
                break;
            }
        }
        if (!found && showDialog) {
            setCompilerPath();
        }
    }
    void setCompilerPath() {
        QString path = QFileDialog::getOpenFileName(
            this,
            "Указать путь к компилятору pawncc.exe",
            "",
            "Компилятор Pawn (pawncc.exe)"
            );
        if (!path.isEmpty()) {
            pawnccPath = path;
            statusBar()->showMessage("Компилятор выбран: " + pawnccPath, 3000);
            QList<QLabel*> labels = statusBar()->findChildren<QLabel*>();
            if (!labels.isEmpty()) {
                labels.first()->setText("Компилятор: " + pawnccPath);
            } else {
                statusBar()->addPermanentWidget(new QLabel("Компилятор: " + pawnccPath));
            }
            saveSettings();
        }
    }
    void updateRecentMenu(QMenu *menu) {
        menu->clear();
        for (const QString &file : recentFiles) {
            QFileInfo fileInfo(file);
            QAction *action = menu->addAction(
                QIcon(":/icons/file.png"),
                fileInfo.fileName() + " - " + fileInfo.absolutePath()
                );
            connect(action, &QAction::triggered, [this, file]() {
                loadFile(file);
            });
        }
        if (recentFiles.isEmpty()) {
            QAction *action = menu->addAction("Нет недавних файлов");
            action->setEnabled(false);
        } else {
            menu->addSeparator();
            QAction *clearAction = menu->addAction("Очистить список");
            connect(clearAction, &QAction::triggered, [this]() {
                recentFiles.clear();
                saveSettings();
                updateRecentMenu(recentMenu);
                if (startPage) {
                    startPage->updateRecentFiles(recentFiles);
                }
            });
        }
    }
    void newFile() {
        if (maybeSave()) {
            CodeEditor* newEditor = new CodeEditor();
            int index = editorTab->addTab(newEditor, "Новый файл");
            editorTab->setCurrentIndex(index);
            currentFile.clear();
            setWindowTitle("PawniX - [Новый файл]");
            stackedWidget->setCurrentIndex(1);
            connect(newEditor->document(), &QTextDocument::contentsChanged, this, &PawnEditor::updateTabTitle);
        }
    }
    void open() {
        if (maybeSave()) {
            QString fileName = QFileDialog::getOpenFileName(this,
                                                            "Открыть файл",
                                                            "",
                                                            "Pawn Files (*.pwn *.inc);;All Files (*.*)");
            if (!fileName.isEmpty()) {
                loadFile(fileName);
            }
        }
    }
    void openWithEncoding() {
        QString fileName = QFileDialog::getOpenFileName(this,
                                                        "Открыть файл",
                                                        "",
                                                        "Pawn Files (*.pwn *.inc);;All Files (*.*)");
        if (fileName.isEmpty()) return;
        QMenu encodingMenu(this);
        encodingMenu.setTitle("Выбор кодировки");
        QAction* windows1251Action = encodingMenu.addAction("Windows-1251");
        QAction* utf8Action = encodingMenu.addAction("UTF-8");
        QAction* selected = encodingMenu.exec(cursor().pos());
        if (!selected) return;
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            QMessageBox::warning(this, "Ошибка", "Не могу открыть файл: " + file.errorString());
            return;
        }
        QByteArray data = file.readAll();
        file.close();
        QString content;
        if (selected == windows1251Action) {
            QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
            if (!codec) {
                QMessageBox::critical(this, "Ошибка", "Кодировка Windows-1251 не поддерживается!");
                return;
            }
            content = codec->toUnicode(data);
        } else if (selected == utf8Action) {
            QTextCodec* codec = QTextCodec::codecForName("UTF-8");
            if (!codec) {
                QMessageBox::critical(this, "Ошибка", "Кодировка UTF-8 не поддерживается!");
                return;
            }
            content = codec->toUnicode(data);
        }
        CodeEditor* newEditor = new CodeEditor();
        newEditor->setPlainText(content);
        int index = editorTab->addTab(newEditor, QFileInfo(fileName).fileName());
        editorTab->setCurrentIndex(index);
        currentFile = fileName;
        updateRecentFilesList(fileName);
        saveSettings();
    }
    void openFolder() {
        QString folderPath = QFileDialog::getExistingDirectory(this, "Открыть папку", "");
        if (!folderPath.isEmpty()) {
            currentFolder = folderPath;
            fsModel->setRootPath(currentFolder);
            fileTree->setRootIndex(fsModel->index(currentFolder));
            fileTree->setVisible(true);
            openFolderBtn->setVisible(false);
            setWindowTitle("PawniX - " + QFileInfo(folderPath).fileName());
            stackedWidget->setCurrentIndex(1);
            saveSettings();
        }
    }
    void compile() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (!currentEditor || currentEditor->toPlainText().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Нет открытого файла для компиляции");
            return;
        }

        if (pawnccPath.isEmpty() || !QFile::exists(pawnccPath)) {
            QMessageBox::warning(this, "Ошибка", "Путь к компилятору pawncc.exe не указан или неверен");
            setCompilerPath();
            return;
        }

        if (currentFile.isEmpty()) {
            QMessageBox::critical(this, "Ошибка", "Сохраните файл перед компиляцией!");
            return;
        }

        if (currentEditor->document()->isModified()) {
            if (!save()) {
                QMessageBox::critical(this, "Ошибка", "Не удалось сохранить файл перед компиляцией!");
                return;
            }
        }

        if (!errorConsole) {
            errorConsole = new QTextEdit();
            errorConsole->setReadOnly(true);
            errorConsole->setStyleSheet("background: #252526; color: #D4D4D4;");
            QDockWidget* dock = new QDockWidget("Консоль", this);
            dock->setWidget(errorConsole);
            addDockWidget(Qt::BottomDockWidgetArea, dock);
        }
        errorConsole->clear();
        errorConsole->append("Начало компиляции...");

        if (pawnProcess) {
            pawnProcess->kill();
            pawnProcess->deleteLater();
            pawnProcess = nullptr;
        }

        pawnProcess = new QProcess(this);
        QString baseDir = QFileInfo(currentFile).absolutePath();
        pawnProcess->setWorkingDirectory(baseDir);

        QStringList includeDirs;
        if (QDir(baseDir + "/pawno/include").exists()) {
            includeDirs << QDir::toNativeSeparators(baseDir + "/pawno/include");
        }
        if (QDir(baseDir + "/include").exists()) {
            includeDirs << QDir::toNativeSeparators(baseDir + "/include");
        }

        QStringList args;
        for (const QString& dir : includeDirs) {
            args << "-i" << dir;
        }
        args << QDir::toNativeSeparators(currentFile);
        args << "-o" + QFileInfo(currentFile).baseName() + ".amx";

        connect(pawnProcess, &QProcess::readyReadStandardOutput, this, &PawnEditor::readOutput);
        connect(pawnProcess, &QProcess::readyReadStandardError, this, &PawnEditor::readError);
        connect(pawnProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &PawnEditor::processFinished);

        pawnProcess->start(pawnccPath, args);
        if (!pawnProcess->waitForStarted(3000)) {
            errorConsole->append("Ошибка: Не удалось запустить компилятор pawncc.exe");
            errorConsole->append("Проверьте путь: " + pawnccPath);
            pawnProcess->deleteLater();
            return;
        }

        if (pawnProcess->state() != QProcess::Running) {
            errorConsole->append("Ошибка: Процесс компилятора не запущен");
            pawnProcess->deleteLater();
            return;
        }
    }
    void readOutput() {
        if (errorConsole && pawnProcess) {
            QByteArray data = pawnProcess->readAllStandardOutput();
            errorConsole->append(QString::fromLocal8Bit(data));
        }
    }
    void readError() {
        if (errorConsole && pawnProcess) {
            QByteArray data = pawnProcess->readAllStandardError();
            errorConsole->append(QString("<font color='white'>") + QString::fromLocal8Bit(data) + "</font>");
        }
    }
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
        if (errorConsole) {
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                errorConsole->append(QString("<font color='green'>") + "Компилация успешно завершена!" + "</font>");
            } else {
                errorConsole->append(QString("<font color='red'>") + "Ошибка компиляции! Код выхода: " + QString::number(exitCode) + "</font>");
            }
        }
        if (pawnProcess) {
            pawnProcess->deleteLater();
            pawnProcess = nullptr;
        }
    }
    void about() {
        QString aboutText =
            "<center><img src=':/icons/pawn_editor.ico' width='64' height='64'></center>"
            "<h2 align='center'>PawniX</h2>"
            "<p align='center'><b>Версия 1.0</b></p>"
            "<p align='center'>Современный редактор для разработки на языке Pawn</p>"
            "<p align='center'>© 2025 kahendrik. Все права защищены.</p>"
            "<p align='center'><b>Сайт разработчика:</b> <a href='https://pawnix.tech'>https://pawnix.tech</a></p>"
            "<p align='center'><b>Исходный код:</b> <a href='https://github.com/kahendrik/pawnix'>https://github.com/kahendrik/pawnix</a></p>"
            "<p align='center'><b>Поддержать разработчика:</b> 2200 7019 1314 4488</a></p>"
            "<p align='center'>Это программное обеспечение распределяется по лицензии MIT.</p>";
        QMessageBox::about(this, "О программе", aboutText);
    }
    void openDocumentation() {
        QDesktopServices::openUrl(QUrl("https://pawnix.gitbook.io/"));
    }
    void find() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (currentEditor) {
            FindDialog dialog(currentEditor, this);
            if (dialog.exec() == QDialog::Accepted) {
                currentEditor->findText(dialog.searchText(), dialog.caseSensitive(), dialog.wholeWords());
            }
        }
    }
    void replace() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (currentEditor) {
            ReplaceDialog dialog(currentEditor, this);
            dialog.exec();
        }
    }
    void goToLine() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (currentEditor) {
            bool ok;
            int lineNumber = QInputDialog::getInt(this, "Перейти к строке",
                                                  "Номер строки:", 1, 1, currentEditor->document()->blockCount(), 1, &ok);
            if (ok) {
                currentEditor->goToLine(lineNumber);
            }
        }
    }
    bool maybeSave() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (!currentEditor || !currentEditor->document()->isModified()) return true;
        QMessageBox::StandardButton ret = QMessageBox::warning(this,
                                                               "Документ изменён",
                                                               "Документ был изменён. Сохранить изменения?",
                                                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) return save();
        if (ret == QMessageBox::Cancel) return false;
        return true;
    }
    void loadFile(const QString& fileName) {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly)) {
            QMessageBox::warning(this, "Ошибка", "Не могу открыть файл: " + file.errorString());
            return;
        }
        QByteArray data = file.readAll();
        file.close();
        QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
        if (!codec) {
            QMessageBox::critical(this, "Ошибка", "Кодировка Windows-1251 не поддерживается!");
            return;
        }
        QString content = codec->toUnicode(data);
        CodeEditor* newEditor = new CodeEditor();
        newEditor->setPlainText(content);
        int index = editorTab->addTab(newEditor, QFileInfo(fileName).fileName());
        editorTab->setCurrentIndex(index);
        currentFile = fileName;
        updateRecentFilesList(fileName);
        saveSettings();
    }
    bool save() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (!currentEditor) return false;
        if (currentFile.isEmpty()) {
            return saveAs();
        }
        return saveFile(currentFile);
    }
    bool saveAs() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (!currentEditor) return false;
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        "Сохранить файл",
                                                        "",
                                                        "Pawn Files (*.pwn *.inc);;All Files (*.*)");
        if (fileName.isEmpty()) return false;
        return saveFile(fileName);
    }
    bool saveFile(const QString& fileName) {
        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            QMessageBox::warning(this, "Ошибка", "Не могу сохранить файл: " + file.errorString());
            return false;
        }
        QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
        if (!codec) {
            QMessageBox::critical(this, "Ошибка", "Кодировка Windows-1251 не поддерживается!");
            return false;
        }
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (!currentEditor) {
            QMessageBox::critical(this, "Ошибка", "Нет активного редактора!");
            return false;
        }
        QByteArray encodedData = codec->fromUnicode(currentEditor->toPlainText());
        file.write(encodedData);
        file.close();
        currentFile = fileName;
        setWindowTitle("PawniX - " + QFileInfo(fileName).fileName());
        currentEditor->document()->setModified(false);
        updateRecentFilesList(fileName);
        return true;
    }
    void updateRecentFilesList(const QString &filePath) {
        if (filePath.isEmpty()) return;
        recentFiles.removeAll(filePath);
        recentFiles.prepend(filePath);
        while (recentFiles.size() > 10) {
            recentFiles.removeLast();
        }
        updateRecentMenu(recentMenu);
        if (startPage) {
            startPage->updateRecentFiles(recentFiles);
        }
        saveSettings();
    }
    void updateWindowTitle() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (currentEditor && currentEditor->document()->isModified()) {
            setWindowTitle("PawniX - " + QFileInfo(currentFile).fileName() + "*");
        } else {
            setWindowTitle("PawniX - " + QFileInfo(currentFile).fileName());
        }
    }
    void onUndo() {
        CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (editor) editor->undo();
    }
    void onRedo() {
        CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (editor) editor->redo();
    }
    void onCut() {
        CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (editor) editor->cut();
    }
    void onCopy() {
        CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (editor) editor->copy();
    }
    void onPaste() {
        CodeEditor* editor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (editor) editor->paste();
    }
    void loadSelectedFile(const QModelIndex &index) {
        QString filePath = fsModel->filePath(index);
        QFileInfo fileInfo(filePath);
        if (fileInfo.isFile() &&
            (fileInfo.suffix().toLower() == "pwn" ||
             fileInfo.suffix().toLower() == "inc" ||
             fileInfo.suffix().toLower() == "txt")) {
            loadFile(filePath);
        } else {
            QMessageBox::warning(this, "Ошибка", "Выбранный элемент не является поддерживаемым файлом Pawn!");
        }
    }
    void closeTab(int index) {
        if (maybeSave()) {
            QWidget* widget = editorTab->widget(index);
            editorTab->removeTab(index);
            delete widget;
        }
    }
    void updateTabTitle() {
        CodeEditor* currentEditor = qobject_cast<CodeEditor*>(editorTab->currentWidget());
        if (currentEditor) {
            int currentIndex = editorTab->currentIndex();
            QString title = QFileInfo(currentFile).fileName();
            if (currentEditor->document()->isModified()) {
                title += "*";
            }
            editorTab->setTabText(currentIndex, title);
            updateWindowTitle();
        }
    }
    QMenu* fileMenu;
    QMenu* recentMenu;
    QMenu* editMenu;
    QMenu* buildMenu;
    QMenu* helpMenu;
};
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Base, QColor("#1E1E1E"));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor("#252526"));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    app.setPalette(darkPalette);
    QFont font;
    font.setFamily("Segoe UI");
    font.setPointSize(10);
    app.setFont(font);
    QSettings settings("kahendrik", "PawniX");
    PawnEditor editor;
    editor.show();
    return app.exec();
}
#include "main.moc"
