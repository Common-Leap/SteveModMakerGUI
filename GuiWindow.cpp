#include "GuiWindow.hpp"

#include "CapeCatalog.hpp"

#include <QComboBox>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QPixmap>
#include <QSize>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTextCursor>
#include <QUrl>
#include <QVBoxLayout>

#ifndef SMM_CLI_BINARY_NAME
#define SMM_CLI_BINARY_NAME "SteveModMaker"
#endif

namespace {
QIcon OfficialCapeIcon(const QString& cape_id) {
	const QImage atlas(QStringLiteral(":/capes/%1.png").arg(cape_id));
	if (atlas.isNull() || atlas.width() < 11 || atlas.height() < 17) {
		return QIcon();
	}

	// Minecraft's outward-facing cape design is the 10x16 region at (1, 1).
	// Keep nearest-neighbor scaling so the selector shows the actual pixel art.
	const QImage back = atlas.copy(1, 1, 10, 16).scaled(
		20, 32, Qt::KeepAspectRatio, Qt::FastTransformation
	);
	return QIcon(QPixmap::fromImage(back));
}

QString SanitizeFolderName(QString folder_name) {
	folder_name = folder_name.trimmed();
	constexpr QChar invalid_chars[] = { '<', '>', ':', '"', '/', '\\', '|', '?', '*' };
	for (const QChar invalid_char : invalid_chars) {
		folder_name.replace(invalid_char, '_');
	}

	for (int i = 0; i < folder_name.size(); ++i) {
		if (folder_name[i].unicode() < 0x20) {
			folder_name[i] = '_';
		}
	}

	while (!folder_name.isEmpty() && (folder_name.endsWith(' ') || folder_name.endsWith('.'))) {
		folder_name.chop(1);
	}
	return folder_name;
}

QString BuildPlayerSlotFolderName(const QString& player_name, int slot_number, bool cape_enabled) {
	const QString sanitized_name = SanitizeFolderName(player_name);
	if (sanitized_name.isEmpty()) {
		return QString();
	}
	return QStringLiteral("(Skin) %1 - slot%2%3")
		.arg(sanitized_name)
		.arg(slot_number)
		.arg(cape_enabled ? QStringLiteral(" - cape") : QString());
}
}

GuiWindow::GuiWindow(QWidget* parent)
	: QWidget(parent)
	, source_mode_input_(new QComboBox(this))
	, username_input_(new QLineEdit(this))
	, skin_file_input_(new QLineEdit(this))
	, skin_file_player_name_input_(new QLineEdit(this))
	, cape_enabled_input_(new QCheckBox("Add Minecraft Cape", this))
	, cape_source_input_(new QComboBox(this))
	, official_cape_input_(new QComboBox(this))
	, cape_file_input_(new QLineEdit(this))
	, special_message_input_(new QLineEdit(this))
	, browse_skin_button_(new QPushButton("Browse Skin...", this))
	, browse_cape_button_(new QPushButton("Browse Cape...", this))
	, slot_input_(new QSpinBox(this))
	, arm_override_input_(new QComboBox(this))
	, output_dir_input_(new QLineEdit(this))
	, browse_output_button_(new QPushButton("Browse...", this))
	, open_output_button_(new QPushButton("Open Folder", this))
	, generate_button_(new QPushButton("Generate Mod Files", this))
	, progress_indicator_(new QProgressBar(this))
	, log_output_(new QPlainTextEdit(this)) {
	setWindowTitle("SteveModMaker");
	resize(860, 700);

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
	skin_file_player_name_input_->setPlaceholderText("Player name for output folder");
	cape_source_input_->addItem("Official Minecraft Cape", "official");
	cape_source_input_->addItem("Minecraft Account Cape", "account");
	cape_source_input_->addItem("Custom Cape PNG", "custom");
	official_cape_input_->setIconSize(QSize(20, 32));
	for (const CapeCatalogEntry& cape : OfficialCapeCatalog()) {
		official_cape_input_->addItem(
			OfficialCapeIcon(QString::fromUtf8(cape.id)),
			QString::fromUtf8(cape.name),
			QString::fromUtf8(cape.id)
		);
	}
	cape_file_input_->setPlaceholderText("Path to a 64x32 Minecraft cape atlas");
	cape_enabled_input_->setToolTip(
		"Creates the cape-enabled model, recovery animations, matching Elytra texture, and cape CSS artwork."
	);
	cape_source_input_->setToolTip(
		"Choose an embedded official cape, the cape equipped by a Minecraft username, or a custom PNG."
	);
	cape_file_input_->setToolTip(
		"Minecraft cape atlas: 64x32 pixels, with the visible 10x16 cape in the standard atlas region."
	);
	special_message_input_->setPlaceholderText("Optional; defaults to username");
	slot_input_->setRange(1, 8);
	slot_input_->setValue(1);

	arm_override_input_->addItem("Auto detect from skin", "");
	arm_override_input_->addItem("Small (Alex)", "small");
	arm_override_input_->addItem("Big (Steve)", "big");

	output_dir_input_->setText(QDir::toNativeSeparators(QDir::currentPath()));
	output_dir_input_->setPlaceholderText("Base folder where <player> will be created");

	auto* skin_file_row = new QWidget(this);
	auto* skin_file_row_layout = new QHBoxLayout(skin_file_row);
	skin_file_row_layout->setContentsMargins(0, 0, 0, 0);
	skin_file_row_layout->setSpacing(6);
	skin_file_row_layout->addWidget(skin_file_input_, 1);
	skin_file_row_layout->addWidget(browse_skin_button_);

	auto* cape_file_row = new QWidget(this);
	auto* cape_file_row_layout = new QHBoxLayout(cape_file_row);
	cape_file_row_layout->setContentsMargins(0, 0, 0, 0);
	cape_file_row_layout->setSpacing(6);
	cape_file_row_layout->addWidget(cape_file_input_, 1);
	cape_file_row_layout->addWidget(browse_cape_button_);

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
	settings_layout->addRow("Skin File Player Name", skin_file_player_name_input_);
	settings_layout->addRow(cape_enabled_input_);
	settings_layout->addRow("Cape Source", cape_source_input_);
	settings_layout->addRow("Official Cape", official_cape_input_);
	settings_layout->addRow("Custom Cape (PNG)", cape_file_row);
	settings_layout->addRow("Boxing Ring Message", special_message_input_);
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
			if (skin_file_player_name_input_->text().trimmed().isEmpty()) {
				skin_file_player_name_input_->setText(QFileInfo(selected).completeBaseName());
			}
		}
	});

	connect(browse_cape_button_, &QPushButton::clicked, this, [this]() {
		const QString selected = QFileDialog::getOpenFileName(
			this,
			"Select Minecraft Cape PNG",
			QDir::fromNativeSeparators(cape_file_input_->text()),
			"PNG Images (*.png);;All Files (*)"
		);
		if (!selected.isEmpty()) {
			cape_file_input_->setText(QDir::toNativeSeparators(selected));
			cape_enabled_input_->setChecked(true);
			cape_source_input_->setCurrentIndex(cape_source_input_->findData("custom"));
		}
	});

	connect(open_output_button_, &QPushButton::clicked, this, [this]() {
		const QString output_dir = QDir::fromNativeSeparators(output_dir_input_->text().trimmed());
		if (output_dir.isEmpty()) {
			return;
		}

		QString target_dir = output_dir;
		const bool use_skin_file = source_mode_input_->currentData().toString() == "file";
		const QString player_folder_input = use_skin_file ? skin_file_player_name_input_->text().trimmed() : username_input_->text().trimmed();
		const QString player_folder_name = BuildPlayerSlotFolderName(
			player_folder_input,
			slot_input_->value(),
			cape_enabled_input_->isChecked()
		);
		if (!player_folder_name.isEmpty()) {
			const QString candidate_output_dir = QDir(output_dir).filePath(player_folder_name);
			if (QFileInfo::exists(candidate_output_dir)) {
				target_dir = candidate_output_dir;
			}
		}

		QDesktopServices::openUrl(QUrl::fromLocalFile(target_dir));
	});

	connect(
		source_mode_input_,
		qOverload<int>(&QComboBox::currentIndexChanged),
		this,
		[this](int) { UpdateSourceModeUi(); }
	);

	connect(cape_enabled_input_, &QCheckBox::toggled, this, [this](bool) { UpdateSourceModeUi(); });
	connect(
		cape_source_input_,
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
	const bool use_cape = cape_enabled_input_->isChecked();
	QString source_input;
	QString player_folder_input;
	QStringList args;
	const int slot_number = slot_input_->value();

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
		player_folder_input = skin_file_player_name_input_->text().trimmed();
		if (player_folder_input.isEmpty()) {
			QMessageBox::warning(this, "Missing Player Name", "Enter a player name for the output folder.");
			return;
		}
		args << "--skin-file" << source_input << "--player-name" << player_folder_input;
	}
	else {
		source_input = username_input_->text().trimmed();
		if (source_input.isEmpty()) {
			QMessageBox::warning(this, "Missing Username", "Enter a Minecraft Java username.");
			return;
		}
		player_folder_input = source_input;
		args << source_input;
	}

	if (use_cape) {
		const QString cape_source = cape_source_input_->currentData().toString();
		if (cape_source == "official") {
			const QString cape_id = official_cape_input_->currentData().toString();
			if (cape_id.isEmpty()) {
				QMessageBox::warning(this, "Missing Official Cape", "Select an official Minecraft cape.");
				return;
			}
			args << "--cape-official" << cape_id;
		}
		else if (cape_source == "custom") {
			const QString cape_input = QDir::fromNativeSeparators(cape_file_input_->text().trimmed());
			if (cape_input.isEmpty()) {
				QMessageBox::warning(this, "Missing Cape File", "Select a custom Minecraft cape PNG.");
				return;
			}
			if (!QFileInfo::exists(cape_input)) {
				QMessageBox::warning(this, "Cape File Missing", "The selected cape file does not exist.");
				return;
			}
			args << "--cape-file" << cape_input;
		}
		else if (cape_source == "account" && use_skin_file) {
			QMessageBox::warning(
				this,
				"Account Cape Unavailable",
				"Minecraft Account Cape requires Minecraft Username as the skin source."
			);
			return;
		}
		else if (cape_source == "account") {
			args << "--cape";
		}
		else {
			QMessageBox::warning(this, "Invalid Cape Source", "Select a cape source.");
			return;
		}
	}

	const QString output_dir = QDir::fromNativeSeparators(output_dir_input_->text().trimmed());
	if (output_dir.isEmpty()) {
		QMessageBox::warning(this, "Missing Output Folder", "Choose where to create the output files.");
		return;
	}

	const QString player_folder_name = BuildPlayerSlotFolderName(player_folder_input, slot_number, use_cape);
	if (player_folder_name.isEmpty()) {
		QMessageBox::warning(this, "Invalid Player Name", "Player name must contain at least one valid folder character.");
		return;
	}

	QDir out_dir(output_dir);
	if (!out_dir.exists() && !QDir().mkpath(output_dir)) {
		QMessageBox::critical(this, "Invalid Output Folder", "Unable to create the selected output folder.");
		return;
	}

	const QString named_output_dir = QDir(output_dir).filePath(player_folder_name);
	const QString special_message = special_message_input_->text();

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

	args << "--patch-subdir" << player_folder_name;
	if (!special_message.trimmed().isEmpty()) {
		args << "--special-message" << special_message;
	}
	args << QString::number(slot_number);
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
		[this, named_output_dir](int exit_code, QProcess::ExitStatus exit_status) {
			if (process_) {
				const QString trailing = QString::fromLocal8Bit(process_->readAll());
				if (!trailing.isEmpty()) {
					AppendLog(trailing);
				}
			}

			if (exit_status == QProcess::NormalExit && exit_code == 0) {
				AppendLog("[GUI] Completed successfully.");
				AppendLog("[GUI] Output folder: " + QDir::toNativeSeparators(named_output_dir));
			} else {
				AppendLog(QString("[GUI] Generator exited with code %1.").arg(exit_code));
			}
			SetBusy(false);
		}
	);

	AppendLog("[GUI] Starting generation...");
	AppendLog("[GUI] Working directory: " + QDir::toNativeSeparators(output_dir));
	AppendLog("[GUI] Source mode: " + (use_skin_file ? QString("local file") : QString("minecraft username")));
	AppendLog("[GUI] Cape mode: " + (use_cape ? QString("enabled") : QString("disabled")));
	if (use_cape) {
		const QString cape_source = cape_source_input_->currentData().toString();
		const QString cape_description = cape_source == "official"
			? official_cape_input_->currentText()
			: (cape_source == "account" ? QString("Minecraft account cape") : QString("custom PNG"));
		AppendLog("[GUI] Cape texture: " + cape_description);
	}
	AppendLog("[GUI] Output subfolder: " + QDir::toNativeSeparators(named_output_dir));
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
	const int account_index = cape_source_input_->findData("account");
	if (auto* model = qobject_cast<QStandardItemModel*>(cape_source_input_->model())) {
		if (QStandardItem* account_item = model->item(account_index)) {
			account_item->setEnabled(!use_skin_file);
		}
	}
	if (use_skin_file && cape_source_input_->currentData().toString() == "account") {
		cape_source_input_->setCurrentIndex(cape_source_input_->findData("official"));
	}

	const bool use_cape = cape_enabled_input_->isChecked();
	const QString cape_source = cape_source_input_->currentData().toString();
	username_input_->setEnabled(!use_skin_file);
	skin_file_input_->setEnabled(use_skin_file);
	skin_file_player_name_input_->setEnabled(use_skin_file);
	browse_skin_button_->setEnabled(use_skin_file);
	cape_enabled_input_->setEnabled(true);
	cape_source_input_->setEnabled(use_cape);
	official_cape_input_->setEnabled(use_cape && cape_source == "official");
	cape_file_input_->setEnabled(use_cape && cape_source == "custom");
	browse_cape_button_->setEnabled(use_cape && cape_source == "custom");
}

void GuiWindow::SetBusy(bool busy) {
	source_mode_input_->setEnabled(!busy);
	username_input_->setEnabled(!busy);
	skin_file_input_->setEnabled(!busy);
	skin_file_player_name_input_->setEnabled(!busy);
	cape_enabled_input_->setEnabled(!busy);
	cape_source_input_->setEnabled(!busy);
	official_cape_input_->setEnabled(!busy);
	cape_file_input_->setEnabled(!busy);
	special_message_input_->setEnabled(!busy);
	browse_skin_button_->setEnabled(!busy);
	browse_cape_button_->setEnabled(!busy);
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
