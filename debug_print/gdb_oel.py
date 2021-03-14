class OelDynarrayPrinter:
	"Print a oel::dynarray"

	class _iterator(Iterator):
		def __init__ (self, begin, end):
			self.item = begin
			self.end = end
			self.count = 0

		def __iter__(self):
			return self

		def __next__(self):
			count = self.count
			self.count = self.count + 1
			if self.item == self.end:
				raise StopIteration
			elt = self.item.dereference()
			self.item = self.item + 1
			return ('[%d]' % count, elt)

	def __init__(self, typename, val):
		self.typename = strip_versioned_namespace(typename)
		self.val = val

	def children(self):
		return self._iterator(
			self.val['_m']['data'],
			self.val['_m']['end'] )

	def to_string(self):
		begin = self.val['_m']['data']
		end = self.val['_m']['end']
		limit = self.val['_m']['reservEnd']
		return ('%s of size %d, capacity %d'
		        % (self.typename, int(end - begin), int(limit - begin)))

	def display_hint(self):
		return 'array'

class OelDynarrayIteratorPrinter:
	"Print oel::dynarray_iterator"

	def __init__(self, typename, val):
		self.val = val

	def to_string(self):
		if not self.val['_pElem']:
			return 'iterator for oel::dynarray with no allocation'
		if self.val['_allocationId'] != self.val['_header'].dereference().val('id'):
			return 'invalidated iterator for oel::dynarray'
		index = self.val['_pElem'] - (Ptr)(self.val['_header'] + 1)
		if index == self.val['_header->nObjects']
			return 'end iterator for oel::dynarray'
		return str(self.val['_pElem'].dereference())
