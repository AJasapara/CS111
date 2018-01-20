#!/usr/bin/python

import sys, string, locale, csv

class Superblock:
	def __init__(self, r):
		self.blocks_count = int(r[1])
		self.inodes_count = int(r[2])
		self.block_size = int(r[3])
		self.inode_size = int(r[4])
		self.blocks_per_group = int(r[5])
		self.inodes_per_group = int(r[6])
		self.first_ino = int(r[7])

class Inode:
	def __init__(self, r):
		self.inode = int(r[1])
		self.type = r[2]
		self.mode = int(r[3])
		self.uid = int(r[4])
		self.gid = int(r[5])
		self.links_count = int(r[6])
		self.ctime = r[7]
		self.mtime = r[8]
		self.atime = r[9]
		self.size = int(r[10])
		self.blocks = int(r[11])
		self.block = []
		for i in xrange(12,24):
			self.block.append(int(r[i]))
		self.ind1 = int(r[24])
		self.ind2 = int(r[25])
		self.ind3 = int(r[26])

class Dirent:
	def __init__(self, r):
		self.p_inode = int(r[1])
		self.offset = int(r[2])
		self.inode = int(r[3])
		self.rec_len = int(r[4])
		self.name_len = int(r[5])
		self.name = r[6].rstrip()

class Indirect:
	def __init__(self, r):
		self.inode = int(r[1])
		self.level = int(r[2])
		self.offset = int(r[3])
		self.block = int(r[4])
		self.ref_block = int(r[5])

if __name__ == '__main__':
	sup = None
	blocks = {}
	bFree = []
	iFree = []
	inodes = []
	inodeNos = []
	dirents = []
	indirects = []
	status = 0

	if(len(sys.argv)) != 2:
		print_error("ERROR: Incorrect arguments. Usage: ./lab3b FILENAME\n") 
	file = open(sys.argv[1], 'r')
	if not file:
		print_error("ERROR: Invalid file.\n")
	f = csv.reader(file)
	for r in f:
		type = r[0]
		if type == 'SUPERBLOCK':
			sup = Superblock(r)
		elif type == 'BFREE':
			bFree.append(int(r[1]))
		elif type == 'IFREE':
			iFree.append(int(r[1]))
		elif type == 'INODE':
			inodes.append(Inode(r))
		elif type == 'DIRENT':
			dirents.append(Dirent(r))
		elif type == 'INDIRECT':
			indirects.append(Indirect(r))

	for i in inodes:
		offset = 0
		for b in i.block:
			if b != 0:
				if b > sup.blocks_count-1:
					print("INVALID BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode, offset))
					status = 2
				elif b < 8:
					print("RESERVED BLOCK {} IN INODE {} AT OFFSET {}".format(b, i.inode, offset))
					status = 2
				elif b in bFree:
					print("ALLOCATED BLOCK {} ON FREELIST".format(b))
					status = 2
				elif b in blocks:
					blocks[b].append((i.inode, offset, ""))
				else:
					blocks[b] = [(i.inode, offset, "")]
			offset += 1
		if i.ind1 != 0:
			if i.ind1 > sup.blocks_count-1:
				print("INVALID INDIRECT BLOCK {} IN INODE {} AT OFFSET 12".format(i.ind1, i.inode))
				status = 2
			elif i.ind1 < 8:
				print("RESERVED INDIRECT BLOCK {} IN INODE {} AT OFFSET 12".format(i.ind1, i.inode))
				status = 2
			elif i.ind1 in bFree:
				print("ALLOCATED BLOCK {} ON FREELIST".format(i.ind1))
				status = 2
			elif i.ind1 in blocks:
					blocks[i.ind1].append((i.inode, 12, "INDIRECT "))
			else:
				blocks[i.ind1] = [(i.inode, 12, "INDIRECT ")]
		if i.ind2 != 0:
			if i.ind2 > sup.blocks_count-1:
				print("INVALID DOUBLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 268".format(i.ind2, i.inode))
				status = 2
			elif i.ind2 < 8:
				print("RESERVED DOUBLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 268".format(i.ind2, i.inode))
				status = 2
			elif i.ind2 in bFree:
				print("ALLOCATED BLOCK {} ON FREELIST".format(i.ind2))
				status = 2
			elif i.ind2 in blocks:
					blocks[i.ind2].append((i.inode, 268, "DOUBLE INDIRECT "))
			else:
				blocks[i.ind2] = [(i.inode, 268, "DOUBLE INDIRECT ")]
		if i.ind3 != 0:
			if i.ind3 > sup.blocks_count-1:
				print("INVALID TRIPLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 65804".format(i.ind3, i.inode))
				status = 2
			elif i.ind3 < 8:
				print("RESERVED TRIPLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 65804".format(i.ind3, i.inode))
				status = 2
			elif i.ind3 in bFree:
				print("ALLOCATED BLOCK {} ON FREELIST".format(i.ind3))
				status = 2
			elif i.ind3 in blocks:
					blocks[i.ind3].append((i.inode, 65804, "TRIPLE INDIRECT "))
			else:
				blocks[i.ind3] = [(i.inode, 65804, "TRIPLE INDIRECT ")]
	for i in indirects:
		s = ""
		if i.level == 1:
			s = "INDIRECT "
		elif i.level == 2:
			s = "DOUBLE INDIRECT "
		elif i.level == 3:
			s = "TRIPLE INDIRECT "

		if i.ref_block != 0:
			if i.ref_block > sup.blocks_count-1:
				print("INVALID {}BLOCK {} IN INODE {} AT OFFSET {}".format(s, i.ref_block, i.inode, i.offset))
				status = 2
			elif i.ref_block < 8:
				print("RESERVED {}BLOCK {} IN INODE {} AT OFFSET {}".format(s, i.ref_block, i.inode, i.offset))
				status = 2
			elif i.ref_block in bFree:
				print("ALLOCATED BLOCK {} ON FREELIST".format(i.ref_block))
				status = 2
			elif i.ref_block in blocks:
					blocks[i.ref_block].append((i.inode, i.offset, s))
			else:
				blocks[i.ref_block] = [(i.inode, i.offset, s)]

	for b in xrange(8, sup.blocks_count):
		if b not in bFree and b not in blocks:
			print("UNREFERENCED BLOCK {}".format(b))
			status = 2
		elif b in blocks and len(blocks[b]) > 1:
			for i, o, s in blocks[b]:
				print("DUPLICATE {}BLOCK {} IN INODE {} AT OFFSET {}".format(s, b, i, o))
				status = 2

	uinodes = iFree
	links = [0] * sup.inodes_count
	parents = [0] * sup.inodes_count
	parents[2] = 2;
	for i in inodes:
		inodeNos.append(i.inode)
		if i.type != '0':
			if i.inode in iFree:
				print("ALLOCATED INODE {} ON FREELIST".format(i.inode))
				status = 2
				uinodes.remove(i.inode)
		elif i.inode not in iFree:
			print("UNALLOCATED INODE {} NOT ON FREELIST".format(i.inode))
			status = 2
			uinodes.append(i.inode)
	for i in xrange(sup.first_ino, sup.inodes_count):
		if i not in iFree and i not in inodeNos:
			print("UNALLOCATED INODE {} NOT ON FREELIST".format(i))
			status = 2
			uinodes.append(i)
	for d in dirents:
		if d.inode <= sup.inodes_count and d.inode not in uinodes:
			if d.name != "'..'" and d.name != "'.'":
				parents[d.inode] = d.p_inode
		if d.inode > sup.inodes_count:
			print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(d.p_inode, d.name, d.inode))
			status = 2
		elif d.inode in uinodes:
			print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(d.p_inode, d.name, d.inode))
		else:
			links[d.inode] += 1
	for i in inodes:
		if i.links_count != links[i.inode]:
			print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(i.inode, links[i.inode], i.links_count))
			status = 2
	for d in dirents:
		if d.name == "'.'" and d.inode != d.p_inode:
			print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(d.p_inode, d.inode, d.p_inode))
			status = 2
		elif d.name == "'..'" and d.inode != parents[d.p_inode]:
			print ("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(d.p_inode, d.inode, parents[d.p_inode]))
			status = 2
	exit(status)