import os
import brotli
import json
import requests
from pydantic import BaseModel
from typing import List, Optional, Union


class EffectValue(BaseModel):
    Values: List[int]
    SkillNames: List[str]
    Extras: List[str]
    BuffName: Optional[str]


class BaseChoice(BaseModel):
    Option: str
    SuccessEffect: str
    FailedEffect: str

    def to_base_translated(self, lang):
        print(self)
        return self


class Choice(BaseChoice):
    SuccessEffectValue: EffectValue
    FailedEffectValue: Optional[EffectValue]

    def to_base_translated(self, lang=0):
        trans_scn = ["速度", "耐力", "力量", "根性", "智力", "技能Pt", "Hint Lv", "体力", "羁绊", "干劲"]
        trans_tcn = ["速度", "耐久力", "力量", "意志力", "智力", "技能Pt", "Hint Lv", "體力", "羈絆", "幹勁"]
        trans_en = ["Speed", "Stamina", "Power", "Guts", "Wisdom", "Skill Pt", "Hint Lv", "Energy", "Bond Gauge", "Motivation"]
        trans_jp = ["スピード", "スタミナ", "パワー", "根性", "賢さ", "スキルPt", "ヒント", "体力", "絆", "やる気"]
        langs = [trans_scn, trans_tcn, trans_en, trans_jp]

        def value_to_string(value: int, is_skill=False):
            if value > 0:
                return f"+{value}"
            elif value < 0:
                return f"{value}"
            if is_skill:
                return "+1"
            return value

        trans = langs[lang]

        success_effects = []
        failed_effects = []

        for_steps = [[success_effects, self.SuccessEffectValue], [failed_effects, self.FailedEffectValue]]

        for eff_list, eff_value in for_steps:
            if not eff_value:
                continue

            for i in range(10):
                if i == 6:
                    continue
                curr_trans = trans[i]
                curr_value = eff_value.Values[i]
                if curr_value == 0:
                    continue
                eff_list.append(f"{curr_trans} {value_to_string(curr_value)}")

            if eff_value.SkillNames:
                for i in eff_value.SkillNames:
                    eff_list.append(f"「{i}」 {trans[6]} {value_to_string(eff_value.Values[6], is_skill=True)}")

            if eff_value.Extras:
                eff_list += eff_value.Extras
            if eff_value.BuffName:
                eff_list.append(f"「{eff_value.BuffName}」獲得")

        return BaseChoice(Option=self.Option,
                          SuccessEffect=f"{'、'.join(success_effects)}",
                          FailedEffect=f"{'、'.join(failed_effects)}")


class DbDataItem(BaseModel):
    Id: int
    Name: str
    TriggerName: str
    Choices: List[List[Union[Choice, BaseChoice]]]


def main():
    with open(input("events.br path: ") or 'events.br', 'rb') as file:
        decompressed_data = brotli.decompress(file.read())
        data_str = decompressed_data.decode()

    events = json.loads(data_str)
    # with open("events.json", "w", encoding="utf8") as f:
    #     f.write(json.dumps(events, indent=4, ensure_ascii=False))

    trans_index = ["scn", "tcn", "en", "jp"]
    for n, lang in enumerate(trans_index):
        events_data = [DbDataItem(**i) for i in events]
        fmt_data = []

        for i in events_data:
            if len(i.Choices) <= 1:
                continue
            i.Choices = [[c.to_base_translated(lang=n) for c in choice] for choice in i.Choices]
            fmt_data.append(i.dict(exclude={"TriggerName"}))

        base_path = "../resources/eventHelper"
        if not os.path.isdir(base_path):
            os.makedirs(base_path)
        with open(f"{base_path}/events_{lang}.json", "w", encoding="utf8") as f:
            f.write(json.dumps(fmt_data, ensure_ascii=False))
        print(f"Generated {base_path}/events_{lang}.json")
    print("Generate done.")


if __name__ == "__main__":
    main()
