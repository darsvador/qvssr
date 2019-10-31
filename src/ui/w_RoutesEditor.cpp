#include "w_RoutesEditor.h"
#include "QvCoreConfigOperations.h"
#include "ui_w_RoutesEditor.h"
#include "w_OutboundEditor.h"
#include "w_JsonEditor.h"
#include "w_InboundEditor.h"

#define CurrentRule this->rules[this->currentRuleIndex]
#define STATUS(msg) ui->statusLabel->setText(tr(msg));

RouteEditor::RouteEditor(QJsonObject connection, QWidget *parent) :
    QDialog(parent),
    root(connection),
    original(connection),
    ui(new Ui::RouteEditor)
{
    // TODO Balancer will not be removed if an rule has been removed.
    ui->setupUi(this);
    //
    inbounds = root["inbounds"].toArray();
    outbounds = root["outbounds"].toArray();
    DomainStrategy = root["routing"].toObject()["domainStrategy"].toString();

    // Applying Balancers.
    for (auto _balancer : root["routing"].toObject()["balancers"].toArray()) {
        auto __balancer = _balancer.toObject();
        Balancers[__balancer["tag"].toString()] = QList<QString>();

        for (auto _ : __balancer["selector"].toArray()) {
            Balancers[__balancer["tag"].toString()].append(_.toString());
        }
    }

    rules = QList<RuleObject>::fromStdList(StructFromJsonString<list<RuleObject>>(JsonToString(root["routing"].toObject()["rules"].toArray())));
    //
    ui->outboundsList->clear();
    ui->inboundsList->clear();

    for (auto out : outbounds) {
        bool hasTag = out.toObject().contains("tag");
        //
        auto protocol = out.toObject()["protocol"].toString();
        auto tag = hasTag ? out.toObject()["tag"].toString() : tr("No Tag");
        //
        ui->outboundsList->addItem(tag + " (" + protocol + ")");
    }

    for (auto in : inbounds) {
        bool hasTag = in.toObject().contains("tag");
        //
        auto tag = hasTag ? in.toObject()["tag"].toString() : tr("No Tag");
        auto protocol = in.toObject()["protocol"].toString();
        auto port = in.toObject()["port"].toVariant().toString();
        //
        auto inItem = new QListWidgetItem(tag + " (" + protocol + ": " + port  + ")");
        inItem->setCheckState(Qt::Unchecked);
        ui->inboundsList->addItem(inItem);
    }

    ui->routesTable->clearContents();

    for (int i = 0; i < rules.size(); i++) {
#define rule rules[i]
        // Set up balancers.

        if (QSTRING(rule.balancerTag).isEmpty()) {
            // Default balancer tag.
            auto bTag = GenerateRandomString();
            rule.balancerTag = bTag.toStdString();
            Balancers[bTag] = QStringList();
        }

        //
        ui->routesTable->insertRow(ui->routesTable->rowCount());
        //
        // WARNING There should be an additional check on the final configuration generation process.
        auto enabledItem = new QTableWidgetItem(tr("Enabled"));
        enabledItem->setCheckState(rule.QV2RAY_RULE_ENABLED ? Qt::Checked : Qt::Unchecked);
        ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 0, enabledItem);
        //
        ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 1, new QTableWidgetItem(rule.inboundTag.size() > 0 ? Stringify(rule.inboundTag) : tr("Any")));
        ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 2, new QTableWidgetItem(QString::number(rule.domain.size() + rule.ip.size()) + " " + tr("Items")));
        ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 3, new QTableWidgetItem(QSTRING(rule.outboundTag)));
#undef rule
    }

    if (rules.size() > 0) {
        ui->routesTable->setCurrentItem(ui->routesTable->item(0, 0));
        currentRuleIndex = 0;
        ShowRuleDetail(CurrentRule);
    } else {
        ui->delRouteBtn->setEnabled(false);

        if (ui->inboundsList->count() > 0) ui->inboundsList->setCurrentRow(0);

        if (ui->outboundsList->count() > 0) ui->outboundsList->setCurrentRow(0);
    }
}

QJsonObject RouteEditor::OpenEditor()
{
    auto result = this->exec();

    if (result == QDialog::Accepted) {
        QJsonArray balancers;

        for (auto item : Balancers) {
            QJsonObject balancerEntry;
            auto key = Balancers.key(item);

            if (!key.isEmpty()) {
                DEBUG(MODULE_UI, "Processing balancer: "  + key.toStdString())

                if (item.size() == 0) {
                    item.append("QV2RAY_EMPTY_SELECTORS");
                    LOG(MODULE_CONFIG, "Adding a default selector list to a balancer with empty selectors.")
                }

                balancerEntry["tag"] = key;
                balancerEntry["selector"] = QJsonArray::fromStringList(item);
                balancers.append(balancerEntry);
            } else {
                LOG(MODULE_UI, "Balancer with an empty key detected! Ignored.")
            }
        }

        //
        QJsonArray rulesArray;

        for (auto _rule : rules) {
            rulesArray.append(GetRootObject(_rule));
        }

        QJsonObject routing;
        routing["domainStrategy"] = DomainStrategy;
        routing["rules"] = rulesArray;
        routing["balancers"] = balancers;
        //
        root["inbounds"] = inbounds;
        root["outbounds"] = outbounds;
        root["routing"] = routing;
        return root;
    } else {
        return original;
    }
}

RouteEditor::~RouteEditor()
{
    delete ui;
}

void RouteEditor::on_buttonBox_accepted()
{
}

void RouteEditor::on_outboundsList_currentRowChanged(int currentRow)
{
    auto outBoundRoot = outbounds[currentRow].toObject();
    auto outboundType = outBoundRoot["protocol"].toString();
    ui->outboundTagLabel->setText(outBoundRoot.contains("tag") ? outBoundRoot["tag"].toString() : tr("No Tag"));
    //
    ui->outboundTypeLabel->setText(outboundType);
    string serverAddress = "N/A";
    string serverPort = "N/A";

    if (outboundType == "vmess") {
        auto x = StructFromJsonString<VMessServerObject>(JsonToString(outBoundRoot["settings"].toObject()["vnext"].toArray().first().toObject()));
        serverAddress = x.address;
        serverPort = to_string(x.port);
    } else if (outboundType == "shadowsocks") {
        auto x = JsonToString(outBoundRoot["settings"].toObject()["servers"].toArray().first().toObject());
        auto Server = StructFromJsonString<ShadowSocksServerObject>(x);
        serverAddress = Server.address;
        serverPort = to_string(Server.port);
    } else if (outboundType == "socks") {
        auto x = JsonToString(outBoundRoot["settings"].toObject()["servers"].toArray().first().toObject());
        auto Server = StructFromJsonString<SocksServerObject>(x);
        serverAddress = Server.address;
        serverPort = to_string(Server.port);
    }

    ui->outboundAddressLabel->setText(QSTRING(serverAddress));
    ui->outboundPortLabel->setText(QSTRING(serverPort));
}

void RouteEditor::on_inboundsList_currentRowChanged(int currentRow)
{
    auto inBoundRoot = inbounds[currentRow].toObject();
    //
    ui->inboundTagLabel->setText(inBoundRoot.contains("tag") ? inBoundRoot["tag"].toString() : tr("No Tag"));
    ui->inboundTypeLabel->setText(inBoundRoot["protocol"].toString());
    ui->inboundAddressLabel->setText(inBoundRoot["listen"].toString());
    ui->inboundPortLabel->setText(inBoundRoot["port"].toVariant().toString());
}

void RouteEditor::ShowRuleDetail(RuleObject rule)
{
    // BUG added the wrong items, should be outbound list.
    ui->balancerSelectionCombo->clear();
    ui->routeOutboundSelector->clear();

    for (auto out : outbounds) {
        ui->routeOutboundSelector->addItem(out.toObject()["tag"].toString());
        ui->balancerSelectionCombo->addItem(out.toObject()["tag"].toString());
    }

    //
    // Balancers combo and balancer list.
    ui->enableBalancerCB->setChecked(rule.QV2RAY_RULE_USE_BALANCER);

    if (!QSTRING(rule.balancerTag).isEmpty()) {
        ui->balancerList->clear();
        ui->balancerList->addItems(Balancers[QSTRING(CurrentRule.balancerTag)]);
    }

    if (!QSTRING(rule.outboundTag).isEmpty()) {
        // Find outbound index by tag.
        auto tag = QSTRING(rule.outboundTag);
        auto index = FindIndexByTag(outbounds, &tag);
        ui->routeOutboundSelector->setCurrentIndex(index);
        //
        // Default balancer tag.
        // Don't add anything, we just show the configuration.
        //auto bTag = GenerateRandomString();
        //rule.balancerTag = bTag.toStdString();
        //Balancers[bTag] = QStringList();
        //LOG(MODULE_UI, "Adding a balancer with tag: " + bTag.toStdString())
    }

    //
    for (int i = 0; i < ui->inboundsList->count(); ++i) {
        ui->inboundsList->item(i)->setCheckState(Qt::Unchecked);
    }

    auto outboundTag = QSTRING(rule.outboundTag);
    int index = FindIndexByTag(outbounds, &outboundTag);
    ui->outboundsList->setCurrentRow(index);
    //
    // Networks
    auto network = QSTRING(rule.network).toLower();
    ui->netUDPRB->setChecked(network.contains("udp"));
    ui->netTCPRB->setChecked(network.contains("tcp"));
    ui->netBothRB->setChecked(network.contains("tcp") && network.contains("udp"));
    //
    // Set protocol checkboxes.
    auto protocols = QList<string>::fromStdList(CurrentRule.protocol);
    ui->routeProtocolHTTPCB->setChecked(protocols.contains("http"));
    ui->routeProtocolTLSCB->setChecked(protocols.contains("tls"));
    ui->routeProtocolBTCB->setChecked(protocols.contains("bittorrent"));
    //
    // Port
    ui->routePortTxt->setText(QSTRING(CurrentRule.port));
    //
    // Users
    QString users = Stringify(CurrentRule.user, NEWLINE);
    ui->routeUserTxt->setPlainText(users);
    //
    // Incoming Sources
    QString sources = Stringify(CurrentRule.source, NEWLINE);
    ui->sourceIPList->setPlainText(sources);
    //
    // Domains
    QString domains = Stringify(CurrentRule.domain, NEWLINE);
    ui->hostList->setPlainText(domains);
    //
    // Outcoming IPs
    QString ips = Stringify(CurrentRule.ip, NEWLINE);
    ui->ipList->setPlainText(ips);

    //
    // Inbound Tags
    if (rule.inboundTag.size() == 0) {
        for (int i = 0; i < ui->inboundsList->count(); i++) {
            ui->inboundsList->item(i)->setCheckState(Qt::Checked);
        }

        ui->inboundsList->setCurrentRow(0);
    } else {
        for (auto inboundTag : rule.inboundTag) {
            auto inTag = QSTRING(inboundTag);
            int _index = FindIndexByTag(inbounds, &inTag);

            // FIXED if an inbound is missing (index out of range)
            if (_index >= 0) {
                if (ui->inboundsList->count() <= _index) {
                    QvMessageBox(this, tr("Route Editor"), tr("Cannot find an inbound by tag: ") + tr("Index Out Of Range"));
                    LOG(MODULE_UI, "FATAL: An inbound could not be found.")
                    return;
                }

                ui->inboundsList->item(_index)->setCheckState(Qt::Checked);
                ui->inboundsList->setCurrentRow(_index);
                STATUS("OK")
            } else {
                STATUS("Cannot find inbound by a tag, possible currupted files?")
                LOG(MODULE_UI, "An inbound could not be determined by tag.")
                return;
            }
        }
    }
}

void RouteEditor::on_editOutboundBtn_clicked()
{
    QJsonObject result;
    int row = ui->outboundsList->currentRow();

    if (row < 0) {
        STATUS("No row selected.")
        LOG(MODULE_UI, "No outbound row selected.")
        return;
    }

    auto currentOutbound = outbounds[row].toObject();
    auto protocol =  currentOutbound["protocol"].toString();

    if (protocol != "vmess" && protocol != "shadowsocks" && protocol != "socks") {
        QvMessageBox(this, tr("Cannot Edit"),
                     tr("This outbound entry is not supported by the GUI editor.") + "\r\n" +
                     tr("We will launch Json Editor instead."));
        JsonEditor *w = new JsonEditor(currentOutbound, this);
        STATUS("Opening JSON editor")
        result = w->OpenEditor();
        delete w;
    } else {
        OutboundEditor *w = new OutboundEditor(currentOutbound, this);
        STATUS("Opening default outbound editor.")
        result = w->OpenEditor();
        delete w;
    }

    outbounds[row] = result;
    on_outboundsList_currentRowChanged(row);
    STATUS("OK")
}

void RouteEditor::on_insertDirectBtn_clicked()
{
    auto freedom = GenerateFreedomOUT("as-is", "", 0);
    auto tag = "Freedom_" + QString::number(QTime::currentTime().msecsSinceStartOfDay());
    auto out = GenerateOutboundEntry("freedom", freedom, QJsonObject(), QJsonObject(), "0.0.0.0", tag);
    this->outbounds.append(out);
    ui->outboundsList->addItem(tag);
    STATUS("Added DIRECT outbound")
}

void RouteEditor::on_editInboundBtn_clicked()
{
    QJsonObject result;
    int row = ui->inboundsList->currentRow();

    if (row < 0) return;

    auto currentInbound = inbounds[row].toObject();
    auto protocol =  currentInbound["protocol"].toString();

    if (protocol != "http" && protocol != "mtproto" && protocol != "socks" && protocol != "dokodemo-door") {
        QvMessageBox(this, tr("Cannot Edit"), tr("Currently, this type of outbound is not supported by the editor.") + "\r\n" +
                     tr("We will launch Json Editor instead."));
        STATUS("Opening JSON editor")
        JsonEditor *w = new JsonEditor(currentInbound, this);
        result = w->OpenEditor();
        delete w;
    } else {
        InboundEditor *w = new InboundEditor(currentInbound, this);
        STATUS("Opening default inbound editor")
        result = w->OpenEditor();
        delete w;
    }

    inbounds[row] = result;
    on_inboundsList_currentRowChanged(row);
    STATUS("OK")
}

void RouteEditor::on_routeProtocolHTTPCB_stateChanged(int arg1)
{
    QList<string> protocols;

    if (arg1 == Qt::Checked) protocols << "http";

    if (ui->routeProtocolTLSCB->isChecked()) protocols << "tls";

    if (ui->routeProtocolBTCB->isChecked()) protocols << "bittorrent";

    CurrentRule.protocol = protocols.toStdList();
    STATUS("Protocol list changed.")
}

void RouteEditor::on_routeProtocolTLSCB_stateChanged(int arg1)
{
    QList<string> protocols;

    if (arg1 == Qt::Checked) protocols << "tls";

    if (ui->routeProtocolHTTPCB->isChecked()) protocols << "http";

    if (ui->routeProtocolBTCB->isChecked()) protocols << "bittorrent";

    CurrentRule.protocol = protocols.toStdList();
    STATUS("Protocol list changed.")
}

void RouteEditor::on_routeProtocolBTCB_stateChanged(int arg1)
{
    QList<string> protocols;

    if (arg1 == Qt::Checked) protocols << "bittorrent";

    if (ui->routeProtocolHTTPCB->isChecked()) protocols << "http";

    if (ui->routeProtocolTLSCB->isChecked()) protocols << "tls";

    CurrentRule.protocol = protocols.toStdList();
    STATUS("Protocol list changed.")
}

void RouteEditor::on_balabcerAddBtn_clicked()
{
    if (!ui->balancerSelectionCombo->currentText().isEmpty()) {
        this->Balancers[QSTRING(CurrentRule.balancerTag)].append(ui->balancerSelectionCombo->currentText());
    }

    ui->balancerList->addItem(ui->balancerSelectionCombo->currentText());
    ui->balancerSelectionCombo->setEditText("");
    STATUS("OK")
}

void RouteEditor::on_balancerDelBtn_clicked()
{
    if (ui->balancerList->currentRow() < 0) {
        return;
    }

    Balancers[QSTRING(CurrentRule.balancerTag)].removeAt(ui->balancerList->currentRow());
    ui->balancerList->takeItem(ui->balancerList->currentRow());
    STATUS("Removed a balancer entry.")
}

void RouteEditor::on_hostList_textChanged()
{
    CurrentRule.domain = SplitLinesStdString(ui->hostList->toPlainText()).toStdList();
}

void RouteEditor::on_ipList_textChanged()
{
    CurrentRule.ip = SplitLinesStdString(ui->ipList->toPlainText()).toStdList();
}

void RouteEditor::on_routePortTxt_textEdited(const QString &arg1)
{
    CurrentRule.port = arg1.toStdString();
}

void RouteEditor::on_routeUserTxt_textEdited(const QString &arg1)
{
    CurrentRule.user = SplitLinesStdString(arg1).toStdList();
}

void RouteEditor::on_routesTable_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn)
    Q_UNUSED(previousColumn)
    Q_UNUSED(previousRow)

    if (currentRow < 0 || currentRow >= rules.size()) {
        DEBUG(MODULE_UI, "Out of range: " + to_string(currentRow))
        return;
    }

    currentRuleIndex = currentRow;
    ShowRuleDetail(CurrentRule);
}

void RouteEditor::on_addRouteBtn_clicked()
{
    // Add Route
    RuleObject rule;
    //
    rule.QV2RAY_RULE_ENABLED = true;
    rule.QV2RAY_RULE_USE_BALANCER = false;
    // Default balancer tag.
    auto bTag = GenerateRandomString();
    rule.balancerTag = bTag.toStdString();
    Balancers[bTag] = QStringList();
    //
    ui->routesTable->insertRow(ui->routesTable->rowCount());
    // WARNING There will be an additional check on the final configuration generation process.
    auto enabledItem = new QTableWidgetItem(tr("Enabled"));
    enabledItem->setCheckState(rule.QV2RAY_RULE_ENABLED ? Qt::Checked : Qt::Unchecked);
    ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 0, enabledItem);
    ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 1, new QTableWidgetItem(rule.inboundTag.size() > 0 ? Stringify(rule.inboundTag) : tr("Any")));
    ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 2, new QTableWidgetItem(QString::number(rule.domain.size() + rule.ip.size()) + " " + tr("Items")));
    ui->routesTable->setItem(ui->routesTable->rowCount() - 1, 3, new QTableWidgetItem(QSTRING(rule.outboundTag)));
    rules.append(rule);
    currentRuleIndex = ui->routesTable->rowCount() - 1;
    ui->routesTable->setCurrentCell(currentRuleIndex, 0);
    ShowRuleDetail(CurrentRule);
    ui->delRouteBtn->setEnabled(true);
}

void RouteEditor::on_changeIOBtn_clicked()
{
    QString outbound = "";

    if (ui->outboundsList->currentRow() < 0) {
        // Don't return as someone may use the outboundTag
        //
        //QvMessageBox(this, tr("Changing route inbound/outbound"), tr("Please select an outbound from the list."));
        //return;
        QvMessageBox(this, tr("Changing route inbound/outbound"),
                     tr("You didn't select an outbound.") + NEWLINE +
                     tr("Banlancer will be used."));
    } else {
        outbound = outbounds[ui->outboundsList->currentRow()].toObject()["tag"].toString();
    }

    QList<string> new_inbounds;
    QList<string> new_inbounds_name;

    for (int i = 0; i < ui->inboundsList->count(); i++) {
        auto _item = ui->inboundsList->item(i);

        if (_item->checkState() == Qt::Checked) {
            // WARN there are possiblilties that someone may forget to set the tag.
            new_inbounds.append(inbounds[i].toObject()["tag"].toString().toStdString());
            new_inbounds_name.append(_item->text().toStdString());
        }
    }

    if (new_inbounds.size() == 0) {
        // TODO what to do?
    }

    if (new_inbounds.contains("")) {
        // Empty tag.
        auto result1 = QvMessageBoxAsk(this, tr("Changing route inbound/outbound"), tr("One or more inbound config(s) have no tag configured, do you still want to continue?"));

        if (result1 != QMessageBox::Yes) {
            return;
        }
    }

    auto result = QvMessageBoxAsk(this, tr("Changing route inbound/outbound"),
                                  tr("Are you sure to change the inbound/outbound of currently selected route?")  + NEWLINE +
                                  tr("Current inbound/outbound combinations:") + NEWLINE + NEWLINE + tr("Inbounds: ") + NEWLINE +
                                  Stringify(new_inbounds_name.toStdList(), NEWLINE) + NEWLINE + tr("Outbound: ") + outbound);

    if (result != QMessageBox::Yes) {
        STATUS("Canceled changing inbound/outbound combination.")
        return;
    }

    CurrentRule.inboundTag = new_inbounds.toStdList();
    CurrentRule.outboundTag = outbound.toStdString();
    STATUS("OK")
}

void RouteEditor::on_routesTable_cellChanged(int row, int column)
{
    if (column != 0) {
        // Impossible
        return;
    }

    if (row < 0) {
        return;
    }

    if (rules.size() <= row) {
        LOG(MODULE_UI, "INFO: This message is possibly caused by adding a new route.")
        LOG(MODULE_UI, "INFO: ... But may indicate to other bugs if you didn't do that.")
        return;
    }

    rules[row].QV2RAY_RULE_ENABLED = ui->routesTable->item(row, column)->checkState() == Qt::Checked;
    STATUS((rules[row].QV2RAY_RULE_ENABLED ? "Enabled a route" : "Disabled a route"))
}

void RouteEditor::on_netBothRB_clicked()
{
    CurrentRule.network = "tcp,udp";
}

void RouteEditor::on_netUDPRB_clicked()
{
    CurrentRule.network = "udp";
}

void RouteEditor::on_netTCPRB_clicked()
{
    CurrentRule.network = "tcp";
}

void RouteEditor::on_routeUserTxt_textChanged()
{
    CurrentRule.user = SplitLinesStdString(ui->routeUserTxt->toPlainText()).toStdList();
}

void RouteEditor::on_sourceIPList_textChanged()
{
    CurrentRule.source = SplitLinesStdString(ui->sourceIPList->toPlainText()).toStdList();
}

void RouteEditor::on_enableBalancerCB_stateChanged(int arg1)
{
    CurrentRule.QV2RAY_RULE_USE_BALANCER = arg1 == Qt::Checked;
    ui->stackedWidget->setCurrentIndex(arg1 == Qt::Checked ? 1 : 0);
}


void RouteEditor::on_routeOutboundSelector_currentIndexChanged(int index)
{
    CurrentRule.outboundTag = outbounds[index].toObject()["tag"].toString().toStdString();
}

void RouteEditor::on_inboundsList_itemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item)
    QList<string> new_inbounds;

    for (int i = 0; i < ui->inboundsList->count(); i++) {
        auto _item = ui->inboundsList->item(i);

        if (_item->checkState() == Qt::Checked) {
            // WARN there are possiblilties that someone may forget to set the tag.
            new_inbounds.append(inbounds[i].toObject()["tag"].toString().toStdString());
        }
    }

    if (new_inbounds.size() == 0) {
        // TODO what to do?
    }

    if (new_inbounds.contains("")) {
        // Empty tag.
        auto result1 = QvMessageBoxAsk(this, tr("Changing route inbound/outbound"), tr("One or more inbound config(s) have no tag configured, do you still want to continue?"));

        if (result1 != QMessageBox::Yes) {
            return;
        }
    }

    CurrentRule.inboundTag = new_inbounds.toStdList();
    STATUS("OK")
}

void RouteEditor::on_delRouteBtn_clicked()
{
    if (ui->routesTable->currentRow() >= 0) {
        auto index = ui->routesTable->currentRow();
        auto rule = rules[index];
        rules.removeAt(index);
        Q_UNUSED(rule)
        ui->routesTable->removeRow(index);

        // Show current row.;
        if (ui->routesTable->rowCount() > 0) {
            currentRuleIndex = 0;
            ui->routesTable->setCurrentCell(currentRuleIndex, 0);
            ShowRuleDetail(CurrentRule);
        }
    }
}

void RouteEditor::on_addDefaultBtn_clicked()
{
    // Add default connection from GlobalConfig
    auto conf = GetGlobalConfig();
    //
    auto _in = conf.inBoundSettings;
    //
    auto _in_httpConf = GenerateHTTPIN(QList<AccountObject>() << _in.httpAccount);
    auto _in_socksConf = GenerateSocksIN((_in.socks_useAuth ? "password" : "noauth"),
                                         QList<AccountObject>() << _in.socksAccount,
                                         _in.socksUDP, QSTRING(_in.socksLocalIP));
    //
    auto _in_HTTP = GenerateInboundEntry(QSTRING(_in.listenip), _in.http_port, "http", _in_httpConf, "HTTP_gConf");
    auto _in_SOCKS = GenerateInboundEntry(QSTRING(_in.listenip), _in.socks_port, "socks", _in_socksConf, "SOCKS_gConf");
    //
    inbounds.append(_in_HTTP);
    ui->inboundsList->addItem("HTTP Global Config");
    ui->inboundsList->item(ui->inboundsList->count() - 1)->setCheckState(Qt::Unchecked);
    //
    inbounds.append(_in_SOCKS);
    ui->inboundsList->addItem("SOCKS Global Config");
    ui->inboundsList->item(ui->inboundsList->count() - 1)->setCheckState(Qt::Unchecked);
}

void RouteEditor::on_insertBlackBtn_clicked()
{
    auto blackHole = GenerateBlackHoleOUT(false);
    auto tag = "blackhole_" + QString::number(QTime::currentTime().msecsSinceStartOfDay());
    auto _blackHoleOutbound = GenerateOutboundEntry("blackhole", blackHole, QJsonObject(), QJsonObject(), "0.0.0.0", tag);
    outbounds.append(_blackHoleOutbound);
    ui->outboundsList->addItem(tag);
}

void RouteEditor::on_delOutboundBtn_clicked()
{
    if (ui->outboundsList->currentRow() < 0) {
        return;
    }

    auto index = ui->outboundsList->currentRow();
    outbounds.removeAt(index);
    ui->outboundsList->takeItem(index);
}

void RouteEditor::on_delInboundBtn_clicked()
{
    if (ui->inboundsList->currentRow() < 0) {
        return;
    }

    auto index = ui->inboundsList->currentRow();
    inbounds.removeAt(index);
    ui->inboundsList->takeItem(index);
}
