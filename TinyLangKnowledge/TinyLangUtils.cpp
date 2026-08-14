#include "TinyLangUtils.h"
#include <QProcess>
#include <QStandardPaths>


void TinyLangUtils::EnsureCLILoaded()
{
    if(!tiny_lang_path.isEmpty() && QFile::exists(tiny_lang_path))
    {
        return;
    }

    tiny_lang_path = QStandardPaths::findExecutable("tinylang");
}

std::expected<void, QString> TinyLangUtils::NewProject(
    const QStringView project_name,
    const QStringView workingDir) // Changed to view
{
    EnsureCLILoaded();

    if(tiny_lang_path.isEmpty())
    {
        return std::unexpected(QObject::tr("The 'tinylang' executable could not be found in the system PATH."));
    }

    QProcess cli;
    if(!workingDir.isEmpty())
    {
        cli.setWorkingDirectory(workingDir.toString());
    }

    cli.start(
        tiny_lang_path,
        {
            QStringLiteral("new"),
            project_name.toString()
        }
    );

    if(!cli.waitForFinished())
    {
        return std::unexpected(QObject::tr("Process failed to execute or timed out."));
    }

    if(cli.exitCode() != 0)
    {
        return std::unexpected(QObject::tr("Process failed with non-zero exit code."));
    }

    return {};
}

static std::expected<std::unique_ptr<QProcess>, QString> RunCliProcess(
    QObject* parent, const QStringList& args, const QString& working_dir = {})
{
    TinyLangUtils::EnsureCLILoaded();
    if(TinyLangUtils::tiny_lang_path.isEmpty())
    {
        return std::unexpected(QObject::tr("The 'tinylang' executable could not be found in the system PATH."));
    }

    auto cli = std::make_unique<QProcess>(parent);
    if(!working_dir.isEmpty())
    {
        cli->setWorkingDirectory(working_dir);
    }

    cli->start(TinyLangUtils::tiny_lang_path, args);
    return cli;
}


static std::expected<QByteArray, QString> RunCliJson(const QStringList& args, const QString& working_dir = {})
{
    TinyLangUtils::EnsureCLILoaded();
    if(TinyLangUtils::tiny_lang_path.isEmpty())
    {
        return std::unexpected(QObject::tr("The 'tinylang' executable could not be found in the system PATH."));
    }

    QProcess cli;
    if(!working_dir.isEmpty())
    {
        cli.setWorkingDirectory(working_dir);
    }

    cli.start(TinyLangUtils::tiny_lang_path, args);
    if(!cli.waitForFinished(5000))
    {
        return std::unexpected(QObject::tr("Process timed out."));
    }

    QByteArray out = cli.readAllStandardOutput();
    if(cli.exitCode() != 0 && out.trimmed().isEmpty())
    {
        QString err = QString::fromUtf8(cli.readAllStandardError()).trimmed();
        return std::unexpected(err.isEmpty() ? QObject::tr("Process failed with non-zero exit code.") : err);
    }

    return out;
}

std::expected<QJsonObject, QString> TinyLangUtils::GetProjectInfo(const QStringView project_path)
{
    auto res = RunCliJson({
        QStringLiteral("info"), project_path.toString(), QStringLiteral("--json")
    });
    if(!res) return std::unexpected(res.error());

    QJsonParseError parse_error{};
    const auto doc = QJsonDocument::fromJson(res.value(), &parse_error);
    if(parse_error.error != QJsonParseError::NoError || !doc.isObject())
    {
        return std::unexpected(QObject::tr("Invalid JSON from tinylang info"));
    }
    return doc.object();
}

std::expected<QJsonArray, QString> TinyLangUtils::CheckProject(const QStringView project_path)
{
    auto res = RunCliJson({
        QStringLiteral("check"), project_path.toString(), QStringLiteral("--json")
    });
    if(!res) return std::unexpected(res.error());

    QJsonParseError parse_error{};
    const auto doc = QJsonDocument::fromJson(res.value(), &parse_error);
    if(parse_error.error != QJsonParseError::NoError || !doc.isArray())
    {
        return std::unexpected(QObject::tr("Invalid JSON from tinylang check"));
    }
    return doc.array();
}

std::expected<QJsonArray, QString> TinyLangUtils::GetSymbols(const QStringView path)
{
    auto res = RunCliJson({
        QStringLiteral("symbols"), path.toString(), QStringLiteral("--json")
    });
    if(!res) return std::unexpected(res.error());

    QJsonParseError parse_error{};
    const auto doc = QJsonDocument::fromJson(res.value(), &parse_error);
    if(parse_error.error != QJsonParseError::NoError || !doc.isArray())
    {
        return std::unexpected(QObject::tr("Invalid JSON from tinylang symbols"));
    }
    return doc.array();
}

std::expected<QJsonObject, QString> TinyLangUtils::FindDefinition(
    const QStringView file_path, const int line, const int col, const QStringView project_path)
{
    QStringList args{
        QStringLiteral("definition"),
        file_path.toString(),
        QString::number(line),
        QString::number(col),
        QStringLiteral("--json")
    };
    if(!project_path.isEmpty())
    {
        args << QStringLiteral("-p") << project_path.toString();
    }

    auto res = RunCliJson(args);
    if(!res) return std::unexpected(res.error());

    QJsonParseError parse_error{};
    const QJsonDocument doc = QJsonDocument::fromJson(res.value(), &parse_error);
    if(parse_error.error != QJsonParseError::NoError || !doc.isObject())
    {
        return std::unexpected(QObject::tr("Invalid JSON from tinylang definition"));
    }
    return doc.object();
}

std::expected<void, QString> TinyLangUtils::BuildProject(const QDir& project_dir, const QDir& output_dir)
{
    QStringList args{
        QStringLiteral("build"),
        project_dir.absolutePath()
    };

    if(!output_dir.absolutePath().isEmpty())
    {
        qDebug() << "build output dir: " << output_dir.absolutePath();
        args << QStringLiteral("-o") << output_dir.absolutePath();
    }

    if(auto res = RunCliJson(args);
        !res)
    {
        return std::unexpected(res.error());
    }

    // no output no need to parse response
    return {};

}

std::expected<std::unique_ptr<QProcess>, QString> TinyLangUtils::RunProject(QObject* parent, const QDir& project_dir)
{
    const QStringList args{
        QStringLiteral("run"),
        project_dir.absolutePath()
    };

    return RunCliProcess(parent, args, project_dir.absolutePath());
}
