#include <QCoreApplication>
#include <QtSql/QtSql>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlTableModel>


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
    QSqlRecord in_record;
    QSqlRecord out_record;

    // This variable controls the size of the accumulation bucket.
    int bucket_size = 10;
    int cur_accumulation =0;
    int low_weight = 0;
    qreal weight_accum = 0.0;
    qreal fee_accum = 0.0;

    for (int i = 0; i < in_model.rowCount(); ++i) {
        in_record = in_model.record(i);
        int weight = in_record.value("weight_lbs").toInt();
        qreal fee = in_record.value("fee").toFloat();
        cur_accumulation = i % bucket_size;
        weight_accum += weight;
        fee_accum += fee;
        qDebug() << weight << fee << "cur_accumulation = " << cur_accumulation
                 << " weight_accum = " << weight_accum;
        if ((i > 0) && (weight % bucket_size) == 0) {
            qDebug() << weight_accum / bucket_size << fee_accum / bucket_size;
            out_record = in_record;
            QSqlField low("weight_lbs_low", QVariant::Int);
            QSqlField high("weight_lbs_high", QVariant::Int);
            low.setValue(low_weight);
            high.setValue(weight);
            out_record.setValue("fee", (fee_accum / bucket_size));
            out_record.replace(out_record.indexOf("weight_lbs_low"), low);
            out_record.setGenerated("weight_lbs_low",true);
            out_record.replace(out_record.indexOf("weight_lbs_high"), high);
            out_record.setGenerated("weight_lbs_high",true);
            out_model.insertRecord(-1,out_record);
            out_model.submit();
            weight_accum = 0.0;
            fee_accum = 0.0;
        } /* endif */
    } /* endfor */
    out_model.submitAll();

    //return a.exec();
    return 0;
}
