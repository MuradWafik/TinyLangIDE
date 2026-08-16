#pragma once
#include <expected>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>



namespace TinyLangUtils
{
struct LintItem
{
    enum class Severity
    {
        warning,
        error,
    };

    QString message;
    int line;
    int col;
    Severity severity;

    QString ToString() const
    {
        const QString severity_str = severity == Severity::warning ?
              QStringLiteral("warning")
            : QStringLiteral("error");

        return QStringLiteral("%1:%2: %3: %4").arg(line).arg(col).arg(severity_str).arg(message);
    }
};

inline QString tiny_lang_path;

void EnsureCLILoaded();
std::expected<void, QString> NewProject(QStringView project_name, QStringView workingDir);
std::expected<QJsonObject, QString> GetProjectInfo(QStringView project_path);
std::expected<QVector<LintItem>, QString>CheckProject(QStringView project_path);
std::expected<QJsonArray, QString> GetSymbols(QStringView path);
std::expected<QJsonObject, QString> FindDefinition(QStringView file_path, int line, int col, QStringView project_path = {});
std::expected<void, QString> BuildProject(const QDir& project_dir, const QDir& output_dir = {});
std::expected<QProcess*, QString> RunProject(QObject* parent, const QDir& project_dir);

static constexpr std::optional<LintItem::Severity> ToSeverity(const QAnyStringView name)
{
    if(name == "warning") return LintItem::Severity::warning;
    if(name == "error") return LintItem::Severity::error;

    return std::nullopt;
}
};
