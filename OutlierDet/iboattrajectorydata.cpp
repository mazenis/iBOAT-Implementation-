#include "iboattrajectorydata.hpp"

IBoatTrajectoryData::IBoatTrajectoryData() {

}

void IBoatTrajectoryData::cleanLists() {
    qDeleteAll(listTrajectories);
    qDeleteAll(listCells);
    listCells.clear();
    listTrajectories.clear();
}

void IBoatTrajectoryData::readHurricaneFromFile(QString filePath) {
    cleanLists();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        qDebug() << "error load file";
        return;
    }
    // skip firts two row
    file.readLine();
    file.readLine();
    qint64 id = 0;
    while(!file.atEnd()) {
        TrajectoryIBoat *newTraj = new TrajectoryIBoat(id);
        QString line = file.readLine();
        QStringList data = line.split(" ");
        for (int i = 2; i < data.length()-1; i+=2) {
            newTraj->listPoints.append(PointFTime(data[i].toDouble(), data[i+1].toDouble(), 0));
        }
        listTrajectories.append(newTraj);
        id++;
    }
    file.close();
}

void IBoatTrajectoryData::readPortoTaxiFromFile(QString filePath) {
    cleanLists();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        qDebug() << "error load file";
        return;
    }
    qint64 id = 0;
    while(!file.atEnd()) {
        TrajectoryIBoat *newTraj = new TrajectoryIBoat(id);
        QString line = file.readLine();
        QStringList data = line.split(";");
        newTraj->timeNum = data[0].toDouble();
        for (qint64 i = 1; i < data.size(); i++) {
            QStringList data2 = data[i].split(",");
            double x = data2[0].toDouble();
            double y = data2[1].toDouble();
            newTraj->listPoints.append(PointFTime(x, y, 0.0));
        }
        listTrajectories.append(newTraj);
        id++;
    }
    file.close();
}

void IBoatTrajectoryData::readBerlinFromFile(QString filePath) {        //Mazen
    cleanLists();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        qDebug() << "error load file";
        return;
    }
    qint64 id = 0;
    file.readLine();
       //QString  StringDate="";

    int Old_Tra_No=1;
    TrajectoryIBoat *tempTr = new TrajectoryIBoat(0);
    while(!file.atEnd()) {

        QString line = file.readLine();
        QStringList data = line.split(",");

        if (data[1].toInt()!=Old_Tra_No)
        {
            TrajectoryIBoat *newTraj = new TrajectoryIBoat(id);
            QString format = "yyyy, dddd, dd hh:mm:ss.zzz";
            QDateTime StartOfTripTime = QDateTime::fromString(data[2], format);

            newTraj->timeNum = StartOfTripTime.secsTo(StartOfTripTime);


            for (qint64 i = 0; i < tempTr->listPoints.count(); i++) {

                double x = tempTr->listPoints.at(i).x();
                double y = tempTr->listPoints.at(i).y();
                newTraj->listPoints.append(PointFTime(x, y, 0.0));
            }

            listTrajectories.append(newTraj);
            id++;
            Old_Tra_No=data[1].toInt();
            tempTr->listPoints.clear();
            double x = data[4].toDouble();
            double y = data[5].toDouble();
            tempTr->listPoints.append(PointFTime(x, y, 0.0));
        }
        else
        {
            double x = data[4].toDouble();
            double y = data[5].toDouble();
            tempTr->listPoints.append(PointFTime(x, y, 0.0));

        }
    }
    file.close();
}

void IBoatTrajectoryData::readBerlinFromFileK(QString filePath) {        //Konarek
    cleanLists();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        qDebug() << "error load file";
        return;
    }

    qint64 countTrip = 0;
    file.readLine();

    QString format = "yyyy-MM-dd hh:mm:ss.zzz";
    int prevTrajId = -1;
    TrajectoryIBoat *newTraj;
   // QDateTime lastTime = QDateTime::fromString("2007-05-28 08:36:47", format);
    while(!file.atEnd()) {

        QString line = file.readLine();
        QStringList data = line.split(",");

        if (data[1].toInt() != prevTrajId)
        {

            newTraj = new TrajectoryIBoat(data[1].toInt());

            QDateTime StartOfTripTime;
            QString TestDate = QString(data[2]);
            if (TestDate.length()==10 || data[3].length()==10)
            {
                continue;
            }
            else
            {
                StartOfTripTime = QDateTime::fromString(data[2], format);
            }


            newTraj->time = StartOfTripTime;

            listTrajectories.append(newTraj);
            prevTrajId = data[1].toInt();
            double x = data[4].toDouble();
            double y = data[5].toDouble();
            newTraj->listPoints.append(PointFTime(x, y, StartOfTripTime));
        }
        else
        {
            QDateTime TripPointTime =QDateTime::fromString(data[2], format);
            double x = data[4].toDouble();
            double y = data[5].toDouble();

            newTraj->listPoints.append(PointFTime(x, y, TripPointTime));
            countTrip++;
        }
    }
    file.close();
}

void IBoatTrajectoryData::mapToCell() {
    foreach (TrajectoryIBoat *traj, listTrajectories) {
        QPoint prevCell(0, 0);
        QPoint cell(0, 0);
        cell.setX(computeIndexX(traj->listPoints.first().x()));
        cell.setY(computeIndexY(traj->listPoints.first().y()));
        QPair<qint64, qint64> cellPair(cell.x(), cell.y());
        listCells[cellPair]->addTraj(0, traj);
        traj->listCells.append(listCells[cellPair]);
        prevCell = cell;
        // position of cell in trajectory
        qint64 pos = 1;
        for (qint64 i = 1; i < traj->listPoints.size(); i++) {
            cell.setX(computeIndexX(traj->listPoints[i].x()));
            cell.setY(computeIndexY(traj->listPoints[i].y()));
            // if point are not map to cells that are not neighbor, we need to compute cells between them
            if (!isCellsNeighbor(prevCell, cell)) {
                QList<QPoint> listAugCell = augumentCell(prevCell, cell);
                // adding augumentet cells to trajectory
                foreach (QPoint cellAug, listAugCell) {
                    QPair<qint64, qint64> cellPairAug(cellAug.x(), cellAug.y());
                    listCells[cellPairAug]->addTraj(pos, traj);
                    traj->listCells.append(listCells[cellPairAug]);
                    pos++;
                }
            }
            // if points are map to cells that are not same, we add them to trajectory
            if (prevCell != cell) {
                QPair<qint64, qint64> cellPair(cell.x(), cell.y());
                listCells[cellPair]->addTraj(pos, traj);
                traj->listCells.append(listCells[cellPair]);
                pos++;
            }
            prevCell = cell;
        }
    }
}

QList<QPoint> IBoatTrajectoryData::augumentCell(QPoint S, QPoint E) {
    QList<QPoint> listAugCell;
    // get coordinates of cell in the middle between cells in function parameters
    QPoint augCell(qRound64((S.x() + E.x()) / 2.0), qRound64((S.y() + E.y()) / 2.0));
    listAugCell.append(augCell);
    // check if augCell is neighbor of S end E cell
    // if not call recursively function
    if (!isCellsNeighbor(S, augCell))
        listAugCell = augumentCell(S, augCell) + listAugCell;
    if (!isCellsNeighbor(augCell, E))
        listAugCell += augumentCell(augCell, E);
    return listAugCell;
}

bool IBoatTrajectoryData::isCellsNeighbor(QPoint cell1, QPoint cell2) {
    if (cell1.x() - 1 == cell2.x() || cell1.x() + 1 == cell2.x() || cell1.x() == cell2.x())
        if (cell1.y() - 1 == cell2.y() || cell1.y() + 1 == cell2.y() || cell1.y() == cell2.y())
            return true;
    return false;
}

qint64 IBoatTrajectoryData:: computeIndexY(double Y) {
    return qRound64((Y - minY) / sizeOfCellY);
}

qint64 IBoatTrajectoryData::computeIndexX(double X) {
    return qRound64((X - minX) / sizeOfCellX);
}

void IBoatTrajectoryData::computeSizeOfCell() {
    sizeOfCellY = qFabs(maxY - minY) / CellNumberY;
    sizeOfCellX = qFabs(maxX - minX) / CellNumberX;
}

void IBoatTrajectoryData::getMinMax() {
    maxX = listTrajectories.first()->listPoints.first().x();
    maxY = listTrajectories.first()->listPoints.first().y();
    minX = listTrajectories.first()->listPoints.first().x();
    minY = listTrajectories.first()->listPoints.first().y();
    foreach (TrajectoryIBoat *traj, listTrajectories) {
        foreach (QPointF point, traj->listPoints) {
            if (maxX < point.x()) maxX = point.x();
            if (maxY < point.y()) maxY = point.y();

            if (minX > point.x()) minX = point.x();
            if (minY > point.y()) minY = point.y();
        }
    }
}

void IBoatTrajectoryData::setSizeOfCell(qint64 CellNumberX, qint64 CellNumberY) {
    this->CellNumberX = CellNumberX;
    this->CellNumberY = CellNumberY;
}

void IBoatTrajectoryData::initCell() {
    getMinMax();
    computeSizeOfCell();
    for(qint64 x = -1; x < CellNumberX+1; x++) {
        for(qint64 y = -1; y < CellNumberY+1; y++) {
            CellIBoat *newCell = new CellIBoat(QPoint(x, y));
            listCells.insert(QPair<qint64, qint64>(x, y), newCell);
        }
    }
    mapToCell();
}
