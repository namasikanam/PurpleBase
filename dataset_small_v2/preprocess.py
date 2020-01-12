tblfiles = ['customer.tbl',
 'lineitem.tbl',
 'nation.tbl',
 'orders.tbl',
 'part.tbl',
 'partsupp.tbl',
 'region.tbl',
 'supplier.tbl']
for filename in tblfiles:
    with open(filename, 'r') as f:
        lines = f.readlines()
    processed_lines = [line.replace('|', ',')[:-2] + "\n" for line in lines]
    with open(filename, 'w') as f:
        f.write(''.join(processed_lines))