import cantools
import json
import sys

def dbc_to_json(dbc_path, json_path):
    print(f"正在加载 DBC 文件: {dbc_path}")
    try:
        db = cantools.database.load_file(dbc_path)
        print(f"DBC 加载成功！找到 {len(db.messages)} 条消息。")
    except Exception as e:
        print(f"❌ DBC 加载失败: {e}")
        sys.exit(1)

    result = {"messages": []}

    for mes in db.messages:
        m = {
            "name": mes.name,
            "id": mes.frame_id,
            "length": mes.length,
            "signals": [] 
        }

        for sig in mes.signals:
            s = {
                "name": sig.name,
                "start_bit": sig.start,
                "length": sig.length,
                "byte_order": "intel" if sig.byte_order == "little_endian" else "motorola",
                "is_signed": sig.is_signed,
                # 强制转换为 float，防止 json.dump 遇到 Decimal 或 None 崩溃
                "factor": float(sig.scale) if sig.scale is not None else 1.0,
                "offset": float(sig.offset) if sig.offset is not None else 0.0,
                "min": float(sig.minimum) if sig.minimum is not None else 0.0,
                "max": float(sig.maximum) if sig.maximum is not None else 0.0,
                "unit": sig.unit if sig.unit else ""
            }
            m["signals"].append(s)

        result["messages"].append(m)

    print("数据组装完毕，正在写入 JSON 文件...")
    try:
        with open(json_path, 'w', encoding='utf-8') as f:
            json.dump(result, f, indent=4, ensure_ascii=False)
        print(f"✅ 转换成功！生成文件: {json_path}")
    except Exception as e:
        print(f"❌ 写入 JSON 时崩溃: {e}")
        sys.exit(1)

if __name__ == "__main__":
    # 检查参数数量
    if len(sys.argv) != 3:
        print("用法: python3 dbc_to_json.py <input.dbc> <output.json>")
        sys.exit(1)
    
    # 接收命令行传入的路径
    input_dbc = sys.argv[1]
    output_json = sys.argv[2]
    
    dbc_to_json(input_dbc, output_json)