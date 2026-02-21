#include "GuiWindow.hpp"

#include <QComboBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

#ifndef SMM_CLI_BINARY_NAME
#define SMM_CLI_BINARY_NAME "SteveModMaker"
#endif

GuiWindow::GuiWindow(QWidget* parent)
	: QWidget(parent)
	, source_mode_input_(new QComboBox(this))
	, username_input_(new QLineEdit(this))
	, skin_file_input_(new QLineEdit(this))
	, browse_skin_button_(new QPushButton("Browse Skin...", this))
	, slot_input_(new QSpinBox(this))
	, arm_override_input_(new QComboBox(this))
	, output_dir_input_(new QLineEdit(this))
	, browse_output_button_(new QPushButton("Browse...", this))
	, open_output_button_(new QPushButton("Open Folder", this))
	, generate_button_(new QPushButton("Generate Mod Files", this))
	, progress_indicator_(new QProgressBar(this))
	, log_output_(new QPlainTextEdit(this)) {
	setWindowTitle("SteveModMaker");
	resize(860, 620);

	auto* root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(16, 16, 16, 16);
	root_layout->setSpacing(10);

	auto* title_label = new QLabel("Steve Mod Maker", this);
	title_label->setObjectName("titleLabel");
	auto* subtitle_label = new QLabel(
		"Generate Super Smash Bros. Ultimate Steve/Alex mod files from a Minecraft username or a local skin file.",
		this
	);
	subtitle_label->setWordWrap(true);
	subtitle_label->setObjectName("subtitleLabel");

	root_layout->addWidget(title_label);
	root_layout->addWidget(subtitle_label);

	auto* settings_group = new QGroupBox("Generation Settings", this);
	auto* settings_layout = new QFormLayout(settings_group);
	settings_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

	source_mode_input_->addItem("Minecraft Username", "username");
	source_mode_input_->addItem("Local Skin File", "file");

	username_input_->setPlaceholderText("Minecraft Java username");
	skin_file_input_->setPlaceholderText("Path to skin PNG");
	slot_input_->setRange(1, 8);
	slot_input_->setValue(1);

	arm_override_input_->addItem("Auto detect from skin", "");
	arm_override_input_->addItem("Small (Alex)", "small");
	arm_override_input_->addItem("Big (Steve)", "big");

	output_dir_input_->setText(QDir::toNativeSeparators(QDir::currentPath()));
	output_dir_input_->setPlaceholderText("Folder where patch/ will be created");

	auto* skin_file_row = new QWidget(this);
	auto* skin_file_row_layout = new QHBoxLayout(skin_file_row);
	skin_file_row_layout->setContentsMargins(0, 0, 0, 0);
	skin_file_row_layout->setSpacing(6);
	skin_file_row_layout->addWidget(skin_file_input_, 1);
	skin_file_row_layout->addWidget(browse_skin_button_);

	auto* output_row = new QWidget(this);
	auto* output_row_layout = new QHBoxLayout(output_row);
	output_row_layout->setContentsMargins(0, 0, 0, 0);
	output_row_layout->setSpacing(6);
	output_row_layout->addWidget(output_dir_input_, 1);
	output_row_layout->addWidget(browse_output_button_);
	output_row_layout->addWidget(open_output_button_);

	settings_layout->addRow("Skin Source", source_mode_input_);
	settings_layout->addRow("Minecraft Username", username_input_);
	settings_layout->addRow("Skin File (PNG)", skin_file_row);
	settings_layout->addRow("Costume Slot", slot_input_);
	settings_layout->addRow("Arm Override", arm_override_input_);
	settings_layout->addRow("Output Folder", output_row);
	root_layout->addWidget(settings_group);

	progress_indicator_->setRange(0, 0);
	progress_indicator_->setVisible(false);
	progress_indicator_->setTextVisible(false);

	auto* actions_layout = new QHBoxLayout();
	actions_layout->setSpacing(8);
	actions_layout->addWidget(generate_button_);
	actions_layout->addWidget(progress_indicator_, 1);
	root_layout->addLayout(actions_layout);

	auto* logs_group = new QGroupBox("Generator Log", this);
	auto* logs_layout = new QVBoxLayout(logs_group);
	log_output_->setReadOnly(true);
	log_output_->setLineWrapMode(QPlainTextEdit::NoWrap);
	logs_layout->addWidget(log_output_);
	root_layout->addWidget(logs_group, 1);

	connect(browse_output_button_, &QPushButton::clicked, this, [this]() {
		const QString selected = QFileDialog::getExistingDirectory(
			this,
			"Select Output Directory",
			QDir::fromNativeSeparators(output_dir_input_->text())
		);
		if (!selected.isEmpty()) {
			output_dir_input_->setText(QDir::toNativeSeparators(selected));
		}
	});

	connect(browse_skin_button_, &QPushButton::clicked, this, [this]() {
		const QString selected = QFileDialog::getOpenFileName(
			this,
			"Select Skin PNG",
			QDir::fromNativeSeparators(skin_file_input_->text()),
			"PNG Images (*.png);;All Files (*)"
		);
		if (!selected.isEmpty()) {
			skin_file_input_->setText(QDir::toNativeSeparators(selected));
		}
	});

	connect(open_output_button_, &QPushButton::clicked, this, [this]() {
		const QString output_dir = QDir::fromNativeSeparators(output_dir_input_->text().trimmed());
		if (output_dir.isEmpty()) {
			return;
		}
		QDesktopServices::openUrl(QUrl::fromLocalFile(output_dir));
	});

	connect(
		source_mode_input_,
		qOverload<int>(&QComboBox::currentIndexChanged),
		this,
		[this](int) { UpdateSourceModeUi(); }
	);

	connect(generate_button_, &QPushButton::clicked, this, &GuiWindow::StartGeneration);

	UpdateSourceModeUi();

	setStyleSheet(
		"QWidget { background: #14171f; color: #e5e9f0; font-size: 13px; }"
		"QLabel#titleLabel { font-size: 26px; font-weight: 700; color: #9cc9ff; }"
		"QLabel#subtitleLabel { color: #a8b2c3; }"
		"QGroupBox { border: 1px solid #3b4557; border-radius: 7px; margin-top: 10px; padding-top: 10px; font-weight: 600; }"
		"QGroupBox::title { left: 10px; padding: 0 4px; color: #c5d2e7; }"
		"QLineEdit, QSpinBox, QComboBox, QPlainTextEdit { background: #1f2530; border: 1px solid #4a566b; border-radius: 6px; padding: 6px; color: #e9eef7; selection-background-color: #3a5372; selection-color: #f7fbff; }"
		"QLineEdit:disabled, QSpinBox:disabled, QComboBox:disabled, QPlainTextEdit:disabled { background: #1a1f29; color: #7f8aa0; }"
		"QPushButton { background: #2e6ba9; color: #f5f8ff; border: none; border-radius: 6px; padding: 8px 14px; font-weight: 600; }"
		"QPushButton:hover { background: #3a80c8; }"
		"QPushButton:pressed { background: #275f96; }"
		"QPushButton:disabled { background: #3b4250; color: #9ca7bb; }"
		"QScrollBar:vertical { background: #1a1f2a; width: 12px; margin: 0px; }"
		"QScrollBar::handle:vertical { background: #4a566b; border-radius: 6px; min-height: 24px; }"
		"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
	);
}

GuiWindow::~GuiWindow() = default;

void GuiWindow::StartGeneration() {
	if (process_ && process_->state() != QProcess::NotRunning) {
		return;
	}

	const bool use_skin_file = source_mode_input_->currentData().toString() == "file";
	QString source_input;
	QStringList args;

	if (use_skin_file) {
		source_input = QDir::fromNativeSeparators(skin_file_input_->text().trimmed());
		if (source_input.isEmpty()) {
			QMessageBox::warning(this, "Missing Skin File", "Select a local skin PNG file.");
			return;
		}
		if (!QFileInfo::exists(source_input)) {
			QMessageBox::warning(this, "Skin File Missing", "The selected skin file does not exist.");
			return;
		}
		args << "--skin-file" << source_input;
	}
	else {
		source_input = username_input_->text().trimmed();
		if (source_input.isEmpty()) {
			QMessageBox::warning(this, "Missing Username", "Enter a Minecraft Java username.");
			return;
		}
		args << source_input;
	}

	const QString output_dir = QDir::fromNativeSeparators(output_dir_input_->text().trimmed());
	if (output_dir.isEmpty()) {
		QMessageBox::warning(this, "Missing Output Folder", "Choose where to create the patch folder.");
		return;
	}

	QDir out_dir(output_dir);
	if (!out_dir.exists() && !QDir().mkpath(output_dir)) {
		QMessageBox::critical(this, "Invalid Output Folder", "Unable to create the selected output folder.");
		return;
	}

	const QString cli_path = ResolveCliExecutable();
	const QFileInfo cli_info(cli_path);
	if (cli_info.isAbsolute() && !cli_info.exists()) {
		QMessageBox::critical(
			this,
			"CLI Binary Not Found",
			"Could not find the SteveModMaker CLI binary next to the GUI executable."
		);
		AppendLog("[GUI] Unable to start: SteveModMaker CLI executable is missing.");
		return;
	}

	args << QString::number(slot_input_->value());
	const QString arm_override = arm_override_input_->currentData().toString();
	if (!arm_override.isEmpty()) {
		args << arm_override;
	}

	process_ = std::make_unique<QProcess>(this);
	process_->setProgram(cli_path);
	process_->setArguments(args);
	process_->setWorkingDirectory(output_dir);
	process_->setProcessChannelMode(QProcess::MergedChannels);

	connect(process_.get(), &QProcess::readyRead, this, [this]() {
		if (!process_) {
			return;
		}
		const QString chunk = QString::fromLocal8Bit(process_->readAll());
		if (!chunk.isEmpty()) {
			AppendLog(chunk);
		}
	});

	connect(process_.get(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
		if (!process_) {
			return;
		}
		AppendLog("[GUI] Failed to run generator: " + process_->errorString());
		SetBusy(false);
	});

	connect(
		process_.get(),
		qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
		this,
		[this, output_dir](int exit_code, QProcess::ExitStatus exit_status) {
			if (process_) {
				const QString trailing = QString::fromLocal8Bit(process_->readAll());
				if (!trailing.isEmpty()) {
					AppendLog(trailing);
				}
			}

			if (exit_status == QProcess::NormalExit && exit_code == 0) {
				AppendLog("[GUI] Completed successfully.");
				AppendLog("[GUI] Output folder: " + QDir::toNativeSeparators(QDir(output_dir).filePath("patch")));
			} else {
				AppendLog(QString("[GUI] Generator exited with code %1.").arg(exit_code));
			}
			SetBusy(false);
		}
	);

	AppendLog("[GUI] Starting generation...");
	AppendLog("[GUI] Working directory: " + QDir::toNativeSeparators(output_dir));
	AppendLog("[GUI] Source mode: " + (use_skin_file ? QString("local file") : QString("minecraft username")));
	AppendLog("[GUI] Command: " + QFileInfo(cli_path).fileName() + " " + args.join(' '));

	SetBusy(true);
	process_->start();
	if (!process_->waitForStarted(3000)) {
		AppendLog("[GUI] Failed to start process: " + process_->errorString());
		SetBusy(false);
	}
}

void GuiWindow::UpdateSourceModeUi() {
	const bool use_skin_file = source_mode_input_->currentData().toString() == "file";
	username_input_->setEnabled(!use_skin_file);
	skin_file_input_->setEnabled(use_skin_file);
	browse_skin_button_->setEnabled(use_skin_file);
}

void GuiWindow::SetBusy(bool busy) {
	source_mode_input_->setEnabled(!busy);
	username_input_->setEnabled(!busy);
	skin_file_input_->setEnabled(!busy);
	browse_skin_button_->setEnabled(!busy);
	slot_input_->setEnabled(!busy);
	arm_override_input_->setEnabled(!busy);
	output_dir_input_->setEnabled(!busy);
	browse_output_button_->setEnabled(!busy);
	generate_button_->setEnabled(!busy);
	progress_indicator_->setVisible(busy);

	if (!busy) {
		UpdateSourceModeUi();
	}
}

void GuiWindow::AppendLog(const QString& message) {
	if (message.isEmpty()) {
		return;
	}

	log_output_->moveCursor(QTextCursor::End);
	log_output_->insertPlainText(message);
	if (!message.endsWith('\n')) {
		log_output_->insertPlainText("\n");
	}
	log_output_->moveCursor(QTextCursor::End);
}

QString GuiWindow::ResolveCliExecutable() const {
	QString binary_name = QStringLiteral(SMM_CLI_BINARY_NAME);
#ifdef Q_OS_WIN
	if (!binary_name.endsWith(".exe", Qt::CaseInsensitive)) {
		binary_name += ".exe";
	}
#endif

	const QDir app_dir(QCoreApplication::applicationDirPath());
	const QString sibling_binary = app_dir.filePath(binary_name);
	if (QFileInfo::exists(sibling_binary)) {
		return sibling_binary;
	}

	// Fallback to PATH lookup in developer environments.
	return binary_name;
}
