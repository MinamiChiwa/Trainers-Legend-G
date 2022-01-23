using Microsoft.Data.Sqlite;
using Newtonsoft.Json;
using System;
using System.IO;

using Dict = System.Collections.Generic.Dictionary<ulong, string>;

namespace TextDumper
{
    class Program
    {
        static void DumpAndUpdateDict(string dictPath, string tableName, string targetField, SqliteConnection conn)
        {
            Dict dict = null;

            if (File.Exists(dictPath))
            {
                dict = JsonConvert.DeserializeObject<Dict>(
                    File.ReadAllText(dictPath)
                );
            }
            else
            {
                dict = new Dict();
            }

            var cmd = conn.CreateCommand();
            cmd.CommandText =
            $@"
                SELECT {targetField}
                FROM {tableName}
            ";

            using (var reader = cmd.ExecuteReader())
            {
                while (reader.Read())
                {
                    var text = reader.GetString(0);
                    var hash = HashLib.HashString(text);

                    if(!dict.ContainsKey(hash))
                        dict.Add(hash, text);
                }
            }


            File.WriteAllText(dictPath, JsonConvert.SerializeObject(dict, new JsonSerializerSettings {
                Formatting = Formatting.Indented
            }));
        }

        static void Main(string[] args)
        {
            var localappdata = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            var dbPath = localappdata + "Low\\Cygames\\umamusume\\master\\master.mdb";

            if (File.Exists(dbPath))
            {
                using (SqliteConnection conn = new($"Data Source={dbPath}"))
                {
                    conn.Open();

                    // Static text need to be dumped from global metadata so we don't dump it here
                    // This only dumps text from database
                    DumpAndUpdateDict("../original_data/common.json", "text_data", "text", conn);
                    DumpAndUpdateDict("../original_data/chara.json", "character_system_text", "text", conn);
                    DumpAndUpdateDict("../original_data/race_comment.json", "race_jikkyo_comment", "message", conn);
                    DumpAndUpdateDict("../original_data/race_message.json", "race_jikkyo_message", "message", conn);
                    
                    conn.Close();
                }
            }
        }
    }
}
