# -*- coding: utf-8 -*-
#!/usr/bin/env python
#================================================================
#   
#   
#   文件名称：crc.py
#   创 建 者：肖飞
#   创建日期：2021年05月08日 星期六 15时24分04秒
#   修改日期：2021年05月08日 星期六 15时37分54秒
#   描    述：
#
#================================================================
import sys
import optparse
import struct

def gen_fw_crc(fw_file):
    content = None
    with open(fw_file, 'rb') as f:
        content = f.read()
    crc = 0
    for i,j in enumerate(content):
        crc += j
    print(hex(crc))
    des = '<I'
    data = struct.pack(des, crc)
    with open('fw.crc', 'wb') as f:
        f.write(data)

def main(argv):
    options = optparse.OptionParser()
    options.add_option('-f', '--file', dest='file', help='file', default=None)
    opts, args = options.parse_args(argv)
    #print('opts:%s' %(opts))
    #print('args:%s' %(args))
    if len(args):
        options.print_help()
        return

    if not opts.file:
        options.print_help()
        return
    gen_fw_crc(opts.file)

if '__main__' == __name__:
    main(sys.argv[1:])
