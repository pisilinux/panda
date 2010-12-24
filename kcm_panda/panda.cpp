#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QProcess>
#include <QTimer>

//Added by qt3to4:
#include <QVBoxLayout>
#include <QFormLayout>
#include <QBoxLayout>
#include <QGroupBox>

#include "helper.h"

#include <kcmodule.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdialog.h>
#include <knotification.h>
#include <klocale.h>
#include <knuminput.h>
#include <kdebug.h>
#include <kmessagebox.h>


// X11 includes
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// OpenGL includes
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>


#include <QX11Info>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include "panda.h"
#include "panda_parser.h"
#include "panda.moc"

K_PLUGIN_FACTORY(PandaConfigFactory, registerPlugin<PandaConfig>();)
K_EXPORT_PLUGIN(PandaConfigFactory("kcmpanda"))


PandaConfig::PandaConfig(QWidget *parent, const QVariantList &args):
    KCModule(PandaConfigFactory::componentData(), parent, args)
{
    
  // The Main Layout, on top of this are two groupboxes, topBox and bottomBox
  QBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0);

  // Current Driver Information
  QGroupBox *topBox = new QGroupBox(i18n("Current Driver Information"), this );

  QGridLayout *infoLayout = new QGridLayout(this);
  infoLayout->setAlignment(Qt::AlignTop|Qt::AlignLeft);

  topBox->setLayout(infoLayout);
  layout->addWidget(topBox);

  QLabel *iconLabel = new QLabel();
  iconLabel->setAlignment(Qt::AlignCenter);
  iconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
  iconLabel->setPixmap(KIcon("hwinfo").pixmap(64));

  // Get opengl informations
  pandaParser = new PandaParser();
  pandaParser->getGlStrings();

  // A simple way to set labels to bold
  QFont bFont;
  bFont.setBold(true);

  // Display driver informations
  QLabel *vendorLabel = new QLabel();
  vendorLabel->setFont(bFont);
  vendorLabel->setText(i18n("Vendor:"));

  QLabel *vendorNameLabel = new QLabel();
  vendorNameLabel->setText(pandaParser->glVendor);
  vendorNameLabel->setIndent(10);

  QLabel *rendererLabel = new QLabel();
  rendererLabel->setFont(bFont);
  rendererLabel->setText(i18n("Renderer:"));

  QLabel *rendererNameLabel = new QLabel();
  rendererNameLabel->setText(pandaParser->glRenderer);
  rendererNameLabel->setIndent(10);

  QLabel *versionLabel = new QLabel();
  versionLabel->setFont(bFont);
  versionLabel->setText(i18n("Version:"));
  QLabel *versionNameLabel = new QLabel();
  versionNameLabel->setText(pandaParser->glVersion);
  versionNameLabel->setIndent(10);

  infoLayout->addWidget(iconLabel,1,1,3,1,Qt::AlignCenter);

  infoLayout->addWidget(vendorLabel,1,2,1,1);
  infoLayout->addWidget(vendorNameLabel,1,3,1,1);

  infoLayout->addWidget(rendererLabel,2,2,1,1);
  infoLayout->addWidget(rendererNameLabel,2,3,1,1);

  infoLayout->addWidget(versionLabel,3,2,1,1);
  infoLayout->addWidget(versionNameLabel,3,3,1,1);

  // Driver Settings
  QGroupBox *bottomGroupBox = new QGroupBox(i18n("Driver Preference"), this );
  QVBoxLayout *layout_settings = new QVBoxLayout();
  bottomGroupBox->setLayout(layout_settings);
  layout->addWidget(bottomGroupBox);
  layout->addStretch();

  osDriver = new QRadioButton(i18n("Use open source driver"), bottomGroupBox);
  vendorDriver = new QRadioButton(i18n("Use vendor driver"), bottomGroupBox);

  QButtonGroup *buttonGroup = new QButtonGroup(bottomGroupBox);
  connect(buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(changed()));

  buttonGroup->setExclusive(true);
  buttonGroup->addButton(osDriver);
  buttonGroup->addButton(vendorDriver);

  layout_settings->addWidget(osDriver);
  layout_settings->addWidget(vendorDriver);

  KAboutData *about =
    new KAboutData(I18N_NOOP("kcmpanda"), 0, ki18n("KDE Panda Control Module"),
                  0, KLocalizedString(), KAboutData::License_GPL,
                  ki18n("(c) 2011 Fatih Arslan"));

  about->addAuthor(ki18n("Fatih Arslan"), ki18n("Original author"), "farslan@pardus.org.tr");
  setAboutData(about);

  // Needed by Kauth, should be executed after KAboutData
  setNeedsAuthorization(true);
}

PandaConfig::~PandaConfig()
{
    delete pandaParser;
}

void PandaConfig::load()
{
  QStringList cliArgs;
  cliArgs << "cur";
  QString program = "/usr/bin/panda-cli.py";

  QProcess *p = new QProcess(this);
  p->start(program, cliArgs);
  p->waitForFinished();

  QByteArray currentDriver = p->readAllStandardOutput();
  QString vendor = "vendor";
  QString os = "os";
  QString pandaOutput = QString(currentDriver).trimmed();

  bool isVendor = (vendor == pandaOutput);
  bool isOs = (os == pandaOutput);

  if (isVendor){
      vendorDriver->setChecked(true);
  } else if (isOs) {
      osDriver->setChecked(true);
  }
}

void PandaConfig::save()
{

  QVariantMap helperargs;

  helperargs["osdriver"] = osDriver->isChecked();
  helperargs["vendordriver"] = vendorDriver->isChecked();

  Action *action = authAction();
  action->setArguments(helperargs);

  ActionReply reply = action->execute();

  if (reply.failed()) {
    if (reply.type() == ActionReply::KAuthError) {
        KMessageBox::error(this, i18n("Unable to authenticate/execute the action: %1, %2", reply.errorCode(), reply.errorDescription()));
    } else {
        KMessageBox::error(this, i18n("Error handler for custom errors should be setup here"));
    }

    QTimer::singleShot(0, this, SLOT(changed()));
  }

}


void PandaConfig::defaults()
{
      osDriver->setChecked(true);
      emit changed(true);
}

