/******************************************************************************************************
 * (C) 2014 markummitchell@github.com. This file is part of Engauge Digitizer, which is released      *
 * under GNU General Public License version 2 (GPLv2) or (at your option) any later version. See file *
 * LICENSE or go to gnu.org/licenses for details. Distribution requires prior written permission.     *
 ******************************************************************************************************/

#include "CmdAddPointGraph.h"
#include "CmdMediator.h"
#include "CursorFactory.h"
#include "DigitizeStateContext.h"
#include "DigitizeStateCurve.h"
#include "Logger.h"
#include "MainWindow.h"
#include "OrdinalGenerator.h"
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QImage>
#include <QMessageBox>
#include <QTextStream>

const char *CSV_FILENAME_DESCRIPTION = "text/csv";
const char *TSV_FILENAME_DESCRIPTION = "text/tsv";
const QString CSV_FILENAME_EXTENSION ("csv");
const QString TSV_FILENAME_EXTENSION ("tsv");

DigitizeStateCurve::DigitizeStateCurve (DigitizeStateContext &context) :
  DigitizeStateAbstractBase (context)
{
}

DigitizeStateCurve::~DigitizeStateCurve ()
{
}

QString DigitizeStateCurve::activeCurve () const
{
  return context().mainWindow().selectedGraphCurve();
}

void DigitizeStateCurve::addGraphCurvePoint (CmdMediator *cmdMediator,
                                             const QPointF &posScreen)
{
  // Create command to add point
  OrdinalGenerator ordinalGenerator;
  Document &document = cmdMediator->document ();
  const Transformation &transformation = context ().mainWindow ().transformation();
  QUndoCommand *cmd = new CmdAddPointGraph (context ().mainWindow(),
                                            document,
                                            context ().mainWindow().selectedGraphCurve(),
                                            posScreen,
                                            ordinalGenerator.generateCurvePointOrdinal(document,
                                                                                       transformation,
                                                                                       posScreen,
                                                                                       activeCurve ()));
  context().appendNewCmd(cmdMediator,
                         cmd);
}

void DigitizeStateCurve::begin (CmdMediator *cmdMediator,
                                DigitizeState /* previousState */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::begin";

  setCursor(cmdMediator);
  context().setDragMode(QGraphicsView::NoDrag);
  context().mainWindow().updateViewsOfSettings(activeCurve ());
}

QCursor DigitizeStateCurve::cursor(CmdMediator *cmdMediator) const
{
  LOG4CPP_DEBUG_S ((*mainCat)) << "DigitizeStateCurve::cursor";

  CursorFactory cursorFactory;
  QCursor cursor = cursorFactory.generate (cmdMediator->document().modelDigitizeCurve());

  return cursor;
}

void DigitizeStateCurve::end ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::end";
}

void DigitizeStateCurve::handleContextMenuEventAxis (CmdMediator * /* cmdMediator */,
                                                     const QString &pointIdentifier)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::handleContextMenuEventAxis "
                              << " point=" << pointIdentifier.toLatin1 ().data ();
}

void DigitizeStateCurve::handleContextMenuEventGraph (CmdMediator * /* cmdMediator */,
                                                      const QStringList &pointIdentifiers)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve ::handleContextMenuEventGraph "
                              << "points=" << pointIdentifiers.join(",").toLatin1 ().data ();
}

void DigitizeStateCurve::handleCurveChange(CmdMediator * /* cmdMediator */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::handleCurveChange";
}

void DigitizeStateCurve::handleFileImportSortedPoints(CmdMediator *cmdMediator)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::handleFileImportSortedPoints";

  // Allow selection of files with strange suffixes in case the file extension was changed. Since
  // the default is the first filter, the wildcard filter is added afterwards (it is the off-nominal case)
  QString filter = QString ("%1 (*.%2);;%3 (*.%4);; All Files (*.*)")
                   .arg (CSV_FILENAME_DESCRIPTION)
                   .arg (CSV_FILENAME_EXTENSION)
                   .arg (TSV_FILENAME_DESCRIPTION)
                   .arg (TSV_FILENAME_EXTENSION);

  QString fileName = QFileDialog::getOpenFileName (0,
                                                   QObject::tr("Import Sorted Points"),
                                                   QDir::currentPath (),
                                                   filter);
  if (!fileName.isEmpty ()) {

    // What was the delimiter
    QString delimiter;
    if (fileName.toLower() .contains (".tsv")) {
      delimiter = '\n';
    } else {
      delimiter = ',';
    }

    // Error flags
    bool isGoodFile = true;
    bool isGoodData = true;

    QFile input (fileName);

    QString line; // High level scope so available in error messages
    if (input.open (QIODevice::ReadOnly)) {

      QTextStream str (&input);
      while (!str.atEnd ()) {
        line = str.readLine();

        QStringList fields = line.split (delimiter);
        if (fields.count () >= 2) {

          // Scrub the data
          bool ok0, ok1;
          int x = fields [0].toInt (&ok0);
          int y = fields [1].toInt (&ok1);
          if (ok0 &&
              ok1 &&
              0 <= x &&
              0 <= y &&
              x < cmdMediator->document().pixmap().width() &&
              y < cmdMediator->document().pixmap().height ()) {

              addGraphCurvePoint (cmdMediator,
                                  QPointF (x, y));

          } else {
            isGoodData = false;
          }

        } else {
          isGoodData = false;
          break;
        }
      }

      input.close ();
    } else {
      isGoodFile = false;
    }

    if (!isGoodFile) {
      QMessageBox::information (0,
                                QObject::tr ("Sorted Points File"),
                                QObject::tr ("Sorted points file could not be opened or read"));
    } else if (!isGoodData) {
      QMessageBox::information (0,
                                QObject::tr ("Sorted Points File"),
                                QObject::tr ("Sorted points line could not be read: ") + line);
    }
  }
}

void DigitizeStateCurve::handleKeyPress (CmdMediator * /* cmdMediator */,
                                         Qt::Key key,
                                         bool /* atLeastOneSelectedItem */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::handleKeyPress"
                              << " key=" << QKeySequence (key).toString ().toLatin1 ().data ();
}

void DigitizeStateCurve::handleMouseMove (CmdMediator * /* cmdMediator */,
                                          QPointF /* posScreen */)
{
//  LOG4CPP_DEBUG_S ((*mainCat)) << "DigitizeStateCurve::handleMouseMove";
}

void DigitizeStateCurve::handleMousePress (CmdMediator * /* cmdMediator */,
                                           QPointF /* posScreen */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::handleMousePress";
}

void DigitizeStateCurve::handleMouseRelease (CmdMediator *cmdMediator,
                                             QPointF posScreen)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::handleMouseRelease";

  addGraphCurvePoint (cmdMediator,
                      posScreen);
}

bool DigitizeStateCurve::isEnableImportSortedPoints() const
{
  return true;
}

QString DigitizeStateCurve::state() const
{
  return "DigitizeStateCurve";
}

void DigitizeStateCurve::updateAfterPointAddition ()
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::updateAfterPointAddition";
}

void DigitizeStateCurve::updateModelDigitizeCurve (CmdMediator *cmdMediator,
                                                   const DocumentModelDigitizeCurve & /*modelDigitizeCurve */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::updateModelDigitizeCurve";

  setCursor(cmdMediator);
}

void DigitizeStateCurve::updateModelSegments(const DocumentModelSegments & /* modelSegments */)
{
  LOG4CPP_INFO_S ((*mainCat)) << "DigitizeStateCurve::updateModelSegments";
}
