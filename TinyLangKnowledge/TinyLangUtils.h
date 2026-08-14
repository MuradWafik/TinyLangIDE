#pragma once
#include <expected>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>


namespace TinyLangUtils
{
    inline QString tiny_lang_path;

    void EnsureCLILoaded();

    std::expected<void, QString> NewProject(QStringView project_name, QStringView workingDir);
    std::expected<QJsonObject, QString> GetProjectInfo(QStringView project_path);
    std::expected<QJsonArray, QString> CheckProject(QStringView project_path);
    std::expected<QJsonArray, QString> GetSymbols(QStringView path);
    std::expected<QJsonObject, QString> FindDefinition(QStringView file_path, int line, int col, QStringView project_path = {});
    std::expected<void, QString> BuildProject(const QDir& project_dir, const QDir& output_dir = {});
    std::expected<std::unique_ptr<QProcess>, QString> RunProject(QObject* parent, const QDir& project_dir);
};
