#!/usr/bin/env python3

import argparse
import os
import sys
import yaml
from jinja2 import Environment, FileSystemLoader


def generate_namespace(protocol, version):
    """Convert protocol and version to C++ namespace."""
    parts = protocol.split('_')
    namespace = '::'.join(parts) + f'::v{version}'
    return namespace


def validate_schema(schema, schema_path):
    """Validate schema structure and content with precise error reporting."""
    
    def error(path, message):
        raise ValueError(f"Schema validation error at {path}: {message}")
    
    # Check top-level structure
    if not isinstance(schema, dict):
        error("root", "Schema must be a dictionary")
    
    # Required top-level keys
    if 'protocol' not in schema:
        error("root", "Missing required field 'protocol'")
    if not isinstance(schema['protocol'], str):
        error("protocol", "Must be a string")
    
    if 'version' not in schema:
        error("root", "Missing required field 'version'")
    if not isinstance(schema['version'], (int, str)):
        error("version", "Must be an integer or string")
    
    if 'messages' not in schema:
        error("root", "Missing required field 'messages'")
    if not isinstance(schema['messages'], dict):
        error("messages", "Must be a dictionary")
    
    # Validate enums if present
    enums = schema.get('enums', {})
    if not isinstance(enums, dict):
        error("enums", "Must be a dictionary")
    
    # Check enum name uniqueness and value types
    enum_names = set()
    for enum_name, enum_def in enums.items():
        if enum_name in enum_names:
            error(f"enums.{enum_name}", "Duplicate enum name")
        enum_names.add(enum_name)
        
        if not isinstance(enum_def, dict):
            error(f"enums.{enum_name}", "Enum definition must be a dictionary")
        
        enum_values = set()
        for value_name, value in enum_def.items():
            if not isinstance(value, (int, str)):
                error(f"enums.{enum_name}.{value_name}", "Enum value must be integer or hex string")
            if isinstance(value, str) and not (value.startswith('0x') or value.startswith('0X')):
                error(f"enums.{enum_name}.{value_name}", "String enum values must be hex (0x...)")
            if value in enum_values:
                error(f"enums.{enum_name}.{value_name}", "Duplicate enum value")
            enum_values.add(value)
    
    # Validate groups if present
    groups = schema.get('groups', {})
    if not isinstance(groups, dict):
        error("groups", "Must be a dictionary")
    
    # Validate type syntax
    def validate_type(field, path):
        type_str = field['type']
        if type_str in ['u8', 'u16', 'u32', 'u64']:
            return
        if type_str == 'char':
            # Check for length field for char arrays
            if 'length' in field:
                try:
                    length = int(field['length'])
                    if length <= 0:
                        error(path, f"char length must be > 0, got {length}")
                except (ValueError, TypeError):
                    error(path, f"Invalid char length: {field['length']}")
            return
        if type_str.startswith('char[') and type_str.endswith(']'):
            try:
                size = int(type_str[5:-1])
                if size <= 0:
                    error(path, f"char array size must be > 0, got {size}")
                return
            except ValueError:
                error(path, f"Invalid char array size in '{type_str}'")
        if type_str == 'enum':
            # Check for enum_type field
            if 'enum_type' in field:
                enum_name = field['enum_type']
                if enum_name not in enum_names:
                    error(path, f"Referenced enum '{enum_name}' does not exist")
            else:
                error(path, "enum type must specify enum_type field")
            return
        if type_str.startswith('enum:'):
            enum_name = type_str[5:]
            if enum_name not in enum_names:
                error(path, f"Referenced enum '{enum_name}' does not exist")
            return
        error(path, f"Invalid type '{type_str}'. Allowed: u8/u16/u32/u64, char, char[N], enum:Name, or enum with enum_type")
    
    # Validate endianness if present
    def validate_endianness(endian_str, path):
        if endian_str not in ['le', 'be']:
            error(path, f"Invalid endianness '{endian_str}'. Must be 'le' or 'be'")
    
    # Get presence map width from type
    def get_presence_map_width(type_str):
        type_widths = {'u8': 8, 'u16': 16, 'u32': 32, 'u64': 64}
        return type_widths.get(type_str, 0)
    
    # Validate group definitions
    for group_name, group_def in groups.items():
        if not isinstance(group_def, dict):
            error(f"groups.{group_name}", "Group definition must be a dictionary")
        
        if 'fields' not in group_def:
            error(f"groups.{group_name}", "Group must have 'fields' list")
        
        fields = group_def['fields']
        if not isinstance(fields, list):
            error(f"groups.{group_name}.fields", "Must be a list")
        
        field_names = set()
        for i, field in enumerate(fields):
            if not isinstance(field, dict):
                error(f"groups.{group_name}.fields[{i}]", "Field must be a dictionary")
            
            if 'name' not in field:
                error(f"groups.{group_name}.fields[{i}]", "Field must have 'name'")
            field_name = field['name']
            
            if field_name in field_names:
                error(f"groups.{group_name}.fields[{i}].name", f"Duplicate field name '{field_name}'")
            field_names.add(field_name)
            
            if 'type' not in field:
                error(f"groups.{group_name}.fields[{i}]", "Field must have 'type'")
            
            validate_type(field, f"groups.{group_name}.fields[{i}].type")
            
            if 'endian' in field:
                validate_endianness(field['endian'], f"groups.{group_name}.fields[{i}].endian")
    
    # Validate messages
    for message_name, message_def in schema['messages'].items():
        if not isinstance(message_def, dict):
            error(f"messages.{message_name}", "Message definition must be a dictionary")
        
        if 'fields' not in message_def:
            error(f"messages.{message_name}", "Message must have 'fields' list")
        
        fields = message_def['fields']
        if not isinstance(fields, list):
            error(f"messages.{message_name}.fields", "Must be a list")
        
        field_names = set()
        presence_map_field = None
        presence_map_width = 0
        
        # First pass: find presence map and validate basic structure
        for i, field in enumerate(fields):
            if not isinstance(field, dict):
                error(f"messages.{message_name}.fields[{i}]", "Field must be a dictionary")
            
            if 'name' not in field:
                error(f"messages.{message_name}.fields[{i}]", "Field must have 'name'")
            field_name = field['name']
            
            if field_name in field_names:
                error(f"messages.{message_name}.fields[{i}].name", f"Duplicate field name '{field_name}'")
            field_names.add(field_name)
            
            # Check for presence map
            if field.get('purpose') == 'presence_map':
                if presence_map_field is not None:
                    error(f"messages.{message_name}.fields[{i}]", "Multiple presence map fields found")
                presence_map_field = field_name
                if 'type' not in field:
                    error(f"messages.{message_name}.fields[{i}]", "Presence map field must have 'type'")
                presence_map_width = get_presence_map_width(field['type'])
                if presence_map_width == 0:
                    error(f"messages.{message_name}.fields[{i}].type", f"Invalid presence map type '{field['type']}'. Must be u8/u16/u32/u64")
        
        # Second pass: validate field types and optional_bit
        for i, field in enumerate(fields):
            field_name = field['name']
            
            # Handle groups section in message (groups defined inline)
            if field_name == 'groups' and 'groups' in message_def:
                # Skip validation for now - this is handled by message-level groups
                continue
            
            else:
                # Regular field - validate type
                if 'type' not in field:
                    error(f"messages.{message_name}.fields[{i}]", "Field must have 'type'")
                
                validate_type(field, f"messages.{message_name}.fields[{i}].type")
                
                if 'endian' in field:
                    validate_endianness(field['endian'], f"messages.{message_name}.fields[{i}].endian")
                
                # Validate optional_bit
                if 'optional_bit' in field:
                    optional_bit = field['optional_bit']
                    if not isinstance(optional_bit, int) or optional_bit < 0:
                        error(f"messages.{message_name}.fields[{i}].optional_bit", "Must be a non-negative integer")
                    if optional_bit >= presence_map_width:
                        error(f"messages.{message_name}.fields[{i}].optional_bit", f"optional_bit {optional_bit} exceeds presence map width {presence_map_width}")
                    if presence_map_field is None:
                        error(f"messages.{message_name}.fields[{i}].optional_bit", "optional_bit specified but no presence map found in message")
        
        # Validate message-level groups if present
        if 'groups' in message_def:
            message_groups = message_def['groups']
            if not isinstance(message_groups, list):
                error(f"messages.{message_name}.groups", "Must be a list")
            
            for group_idx, group in enumerate(message_groups):
                if not isinstance(group, dict):
                    error(f"messages.{message_name}.groups[{group_idx}]", "Group must be a dictionary")
                
                if 'name' not in group:
                    error(f"messages.{message_name}.groups[{group_idx}]", "Group must have 'name'")
                
                if 'count_field' in group:
                    count_field = group['count_field']
                    if count_field not in field_names:
                        error(f"messages.{message_name}.groups[{group_idx}].count_field", f"Count field '{count_field}' not found in message fields")
                
                if 'fields' in group:
                    group_fields = group['fields']
                    if not isinstance(group_fields, list):
                        error(f"messages.{message_name}.groups[{group_idx}].fields", "Must be a list")
                    
                    group_field_names = set()
                    for field_idx, group_field in enumerate(group_fields):
                        if not isinstance(group_field, dict):
                            error(f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}]", "Field must be a dictionary")
                        
                        if 'name' not in group_field:
                            error(f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}]", "Field must have 'name'")
                        
                        group_field_name = group_field['name']
                        if group_field_name in group_field_names:
                            error(f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}].name", f"Duplicate field name '{group_field_name}'")
                        group_field_names.add(group_field_name)
                        
                        if 'type' not in group_field:
                            error(f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}]", "Field must have 'type'")
                        
                        validate_type(group_field, f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}].type")
                        
                        if 'endian' in group_field:
                            validate_endianness(group_field['endian'], f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}].endian")
                        
                        # Validate optional_bit for group fields
                        if 'optional_bit' in group_field:
                            optional_bit = group_field['optional_bit']
                            if not isinstance(optional_bit, int) or optional_bit < 0:
                                error(f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}].optional_bit", "Must be a non-negative integer")
                            if optional_bit >= presence_map_width:
                                error(f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}].optional_bit", f"optional_bit {optional_bit} exceeds presence map width {presence_map_width}")
                            if presence_map_field is None:
                                error(f"messages.{message_name}.groups[{group_idx}].fields[{field_idx}].optional_bit", "optional_bit specified but no presence map found in message")

def build_model(schema):
    """Build a generation model with computed enum widths and message metadata."""
    def parse_enum_value(v):
        if isinstance(v, int):
            return v
        if isinstance(v, str):
            return int(v, 16) if v.lower().startswith("0x") else int(v)
        raise ValueError(f"Unsupported enum value type: {type(v)}")

    # Enums info
    enums_info = {}
    for enum_name, enum_def in schema.get('enums', {}).items():
        values = {k: parse_enum_value(v) for k, v in enum_def.items()}
        max_val = max(values.values()) if values else 0
        if max_val <= 0xFF:
            underlying = 'uint8_t'
            width_bytes = 1
        elif max_val <= 0xFFFF:
            underlying = 'uint16_t'
            width_bytes = 2
        elif max_val <= 0xFFFFFFFF:
            underlying = 'uint32_t'
            width_bytes = 4
        else:
            underlying = 'uint64_t'
            width_bytes = 8
        enums_info[enum_name] = {
            'name': enum_name,
            'underlying': underlying,
            'width_bytes': width_bytes,
            'values': values,
        }

    # Helper to compute field size and c++ type
    def field_cxx_type(field):
        t = field['type']
        if t == 'u8':
            return 'uint8_t'
        if t == 'u16':
            return 'uint16_t'
        if t == 'u32':
            return 'uint32_t'
        if t == 'u64':
            return 'uint64_t'
        if t == 'char':
            if 'length' in field:
                return f"std::array<char, {int(field['length'])}>"
            return 'char'
        if t == 'enum':
            return field['enum_type']
        if t.startswith('enum:'):
            return t.split(':', 1)[1]
        # Allow numeric types carrying enum_type for user convenience
        if 'enum_type' in field and t in ['u8','u16','u32','u64']:
            return t
        return t

    def field_size_bytes(field):
        t = field['type']
        if t == 'u8':
            return 1
        if t == 'u16':
            return 2
        if t == 'u32':
            return 4
        if t == 'u64':
            return 8
        if t == 'char':
            return int(field.get('length', 1))
        if t == 'enum':
            enum_name = field['enum_type']
            return enums_info[enum_name]['width_bytes']
        if t.startswith('enum:'):
            enum_name = t.split(':', 1)[1]
            return enums_info[enum_name]['width_bytes']
        # If user uses primitive with enum_type, size is primitive
        if t in ['u8','u16','u32','u64']:
            return {'u8':1,'u16':2,'u32':4,'u64':8}[t]
        raise ValueError(f"Unsupported field type for size: {t}")

    # Messages info
    messages_info = []
    for msg_name, msg_def in schema.get('messages', {}).items():
        fields = msg_def.get('fields', [])
        presence_field = None
        presence_width = 0
        for f in fields:
            if f.get('purpose') == 'presence_map':
                presence_field = f['name']
                t = f['type']
                presence_width = {'u8':8,'u16':16,'u32':32,'u64':64}.get(t, 0)
                break

        # detect length field heuristically
        length_field_name = None
        for f in fields:
            n = f.get('name','')
            if n in ('MessageLength','Length','MsgLength','MessageSize') and f.get('type') in ('u8','u16','u32','u64'):
                length_field_name = n
                break

        # model fields
        model_fields = []
        for f in fields:
            mf = {
                'name': f['name'],
                'type': f['type'],
                'cxx_type': field_cxx_type(f),
                'size': field_size_bytes(f),
                'endian': f.get('endian'),
                'has_value': 'value' in f,
                'value': f.get('value'),
                'optional_bit': f.get('optional_bit'),
                'is_presence_map': f.get('purpose') == 'presence_map',
                'enum_type': f.get('enum_type'),
            }
            model_fields.append(mf)

        # groups info
        groups_info = []
        for g in msg_def.get('groups', []) or []:
            group_fields = []
            for gf in g.get('fields', []):
                mgf = {
                    'name': gf['name'],
                    'type': gf['type'],
                    'cxx_type': field_cxx_type(gf),
                    'size': field_size_bytes(gf),
                    'endian': gf.get('endian'),
                    'has_value': 'value' in gf,
                    'value': gf.get('value'),
                    'optional_bit': gf.get('optional_bit'),
                    'enum_type': gf.get('enum_type'),
                }
                group_fields.append(mgf)
            vec_name = g['name'].lower()
            groups_info.append({
                'name': g['name'],
                'vector_name': vec_name,
                'count_field': g.get('count_field'),
                'fields': group_fields,
            })
        count_field_map = {g['count_field']: g['vector_name'] for g in groups_info if g.get('count_field')}

        # compute fixed bytes (fields without optional bits and excluding groups)
        fixed_bytes = 0
        has_optional = False
        for f in model_fields:
            if f['optional_bit'] is not None:
                has_optional = True
            else:
                fixed_bytes += f['size']

        messages_info.append({
            'name': msg_name,
            'fields': model_fields,
            'presence_field': presence_field,
            'presence_width': presence_width,
            'length_field': length_field_name,
            'groups': groups_info,
            'count_field_map': count_field_map,
            'fixed_bytes': fixed_bytes,
            'has_optional': has_optional,
            'has_groups': bool(groups_info),
        })

    return {
        'enums': list(enums_info.values()),
        'enums_map': enums_info,
        'messages': messages_info,
    }


def main():
    parser = argparse.ArgumentParser(description='Generate C++ code from YAML schema')
    parser.add_argument('--schema', required=True, help='Input YAML schema file')
    parser.add_argument('--out', required=True, help='Output directory')
    
    args = parser.parse_args()
    
    # Read and parse YAML schema
    try:
        with open(args.schema, 'r') as f:
            schema = yaml.safe_load(f)
    except Exception as e:
        print(f"Error reading schema: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Validate schema
    try:
        validate_schema(schema, args.schema)
        print(f"Schema OK: {args.schema}")
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Extract protocol info
    protocol = schema.get('protocol', 'unknown')
    version = schema.get('version', 1)
    namespace = generate_namespace(protocol, version)
    
    # Setup Jinja2 environment
    env = Environment(
        loader=FileSystemLoader('codegen/templates'),
        trim_blocks=True,
        lstrip_blocks=True
    )
    
    # Build generation model with computed metadata
    model = build_model(schema)
    
    # Template context
    context = {
        'schema': schema,
        'protocol': protocol,
        'version': version,
        'namespace': namespace,
        'model': model
    }
    
    # Template files to generate
    templates = [
        'messages.hpp.j2',
        'messages.cpp.j2',
        'encoder.hpp.j2',
        'encoder.cpp.j2',
        'decoder.hpp.j2',
        'decoder.cpp.j2',
        'handler.hpp.j2'
    ]
    
    # Create output directory
    os.makedirs(args.out, exist_ok=True)
    
    # Generate files
    for template_name in templates:
        try:
            template = env.get_template(template_name)
            output = template.render(**context)
            
            # Remove .j2 extension for output filename
            output_file = os.path.join(args.out, template_name[:-3])
            
            with open(output_file, 'w') as f:
                f.write(output)
                
        except Exception as e:
            print(f"Error processing template {template_name}: {e}", file=sys.stderr)
            sys.exit(1)
    
    print(f"Generated to {args.out}")


if __name__ == '__main__':
    main()
