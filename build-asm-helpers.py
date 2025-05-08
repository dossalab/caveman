from jinja2 import Template
from argparse import ArgumentParser

filename = 'generated-asm.h'

p = ArgumentParser()
p.add_argument('--line-buffer-size', type=int, required=True)
args = p.parse_args()

with open(f'{filename}.j2') as f:
    template = Template(f.read())

with open(filename, 'w') as f:
    f.write(template.render({'line_buffer_size' : args.line_buffer_size}))
