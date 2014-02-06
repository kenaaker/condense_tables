#include <QCoreApplication>
#include <QtSql/QtSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlTableModel>
#include <iostream>

void update_table(int low_weight, int high_weight, float avg_weight, float avg_fee,
                  QSqlTableModel &out_model, QSqlRecord &in_record, QSqlRecord &out_record) {
    qDebug() << avg_weight << avg_fee;
    out_record.setValue("fee", avg_fee);
    out_record.setValue("ship_type", in_record.value("ship_type"));
    out_record.setValue("zone_id", in_record.value("zone_id"));
    out_record.setValue("weight_lbs_low", low_weight);
    out_record.setGenerated("weight_lbs_low",true);
    out_record.setValue("weight_lbs_high", high_weight);
    out_record.setGenerated("weight_lbs_high",true);
    if (!out_model.insertRecord(-1,out_record)) {
        std::cout << "Database Write Error" << " The database reported an error: " <<
                     out_model.lastError().text().toStdString();
    } /* endif */
    out_model.submit();
} /* update_table */

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("localhost");
    db.setDatabaseName("kaaker");
    db.setUserName(getenv("USER"));
    db.setPassword("var6look");
    bool ok = db.open();
    if (!ok) {

    }

    QSqlTableModel in_model;
    in_model.setTable("UPS_consolidated_shipping_zone_fee");
    in_model.setSort(2,Qt::AscendingOrder);
    in_model.select();
    QSqlTableModel out_model;
    out_model.setTable("UPS_condensed_shipping_zone_fee");
    out_model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    QSqlRecord in_record = in_model.record();
    QSqlRecord out_record = out_model.record();

    // This variable controls the size of the accumulation bucket.
    int bucket_boundary = 10;
    int samples_count = 0;
    int low_weight = in_model.record(0).value("weight_lbs").toInt();
    int high_weight = -1;
    qreal weight_accum = 0.0;
    qreal fee_accum = 0.0;

    for (int i = 0; i < in_model.rowCount(); ++i) {
        in_record = in_model.record(i);
        int weight = in_record.value("weight_lbs").toInt();
        qreal fee = in_record.value("fee").toFloat();
        if (weight < high_weight) { /* Reached a zone boundary, make a record and reset */
            if (samples_count > 0) {
                update_table(low_weight, high_weight, weight_accum/samples_count, fee_accum/samples_count,
                             out_model, in_record, out_record);
                weight_accum = 0;
                fee_accum = 0;
                samples_count = 0;
            } /* endif */
            low_weight = weight;
        } /* endif */
        weight_accum += weight;
        fee_accum += fee;
        ++samples_count;
        high_weight = weight;
        if ((weight > 0) && (weight % bucket_boundary) == 0) { /* Reached a zone boundary, reset the accumulation */
            if (samples_count > 0) {
                update_table(low_weight, high_weight, weight_accum/samples_count, fee_accum/samples_count,
                             out_model, in_record, out_record);
            } /* endif */
            low_weight = weight;
            high_weight = weight;
            weight_accum = 0;
            fee_accum = 0;
            samples_count = 0;
        } /* endif */
    } /* endfor */
    if (samples_count > 0) {
        update_table(low_weight, high_weight, weight_accum/samples_count, fee_accum/samples_count,
                     out_model, in_record, out_record);
    } /* endif */
    if (!out_model.submitAll()) {
       std::cout << "Database Write Error" << " The database reported an error: " <<
                    out_model.lastError().text().toStdString() << std::endl;
    } /* endif */

    //return a.exec();
    return 0;
}
