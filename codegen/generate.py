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
    
    # Template context
    context = {
        'schema': schema,
        'protocol': protocol,
        'version': version,
        'namespace': namespace
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
