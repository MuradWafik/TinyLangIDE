#pragma once
#include <qdir.h>
#include <expected>


namespace TinyLangUtils
{
   inline QString tiny_lang_path;

   void EnsureCLILoaded();

   std::expected<void, QString> NewProject(QStringView project_name, QStringView workingDir);

};
