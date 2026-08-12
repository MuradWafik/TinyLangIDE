#include "TinyLangUtils.h"
#include <QStandardPaths>
#include <QProcess>


void TinyLangUtils::EnsureCLILoaded()
{
    if(!tiny_lang_path.isEmpty())
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
        return std::unexpected("The 'tinylang' executable could not be found in the system PATH.");
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
        return std::unexpected("Process failed to execute or timed out.");
    }

    if(cli.exitCode() != 0)
    {
        return std::unexpected("Process failed with non-zero exit code.");
    }

    return {};
}
