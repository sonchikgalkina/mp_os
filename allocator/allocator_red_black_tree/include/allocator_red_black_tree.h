#include <not_implemented.h>

#include "../include/allocator_red_black_tree.h"

std::string rb_allocator = "[RB allocator] ";

allocator_red_black_tree::~allocator_red_black_tree()
{
    debug_with_guard(get_typename() + "In destructor\n");
	get_mutex().~mutex();
	deallocate_with_guard(_trusted_memory);
}

allocator_red_black_tree::allocator_red_black_tree(
    allocator_red_black_tree &&other) noexcept : _trusted_memory(other._trusted_memory)
{
    debug_with_guard(get_typename() + "In move constructor\n");
	other._trusted_memory = nullptr;
}

allocator_red_black_tree &allocator_red_black_tree::operator=(
    allocator_red_black_tree &&other) noexcept
{
    debug_with_guard(get_typename() + "In assignment operator\n");
	if (this != &other)
	{
		get_mutex().~mutex();
		deallocate_with_guard(_trusted_memory);

		_trusted_memory = other._trusted_memory;
		other._trusted_memory = nullptr;
	}
	return *this;
}

allocator_red_black_tree::allocator_red_black_tree(
    size_t space_size,
    allocator *parent_allocator,
    logger *logger,
    allocator_with_fit_mode::fit_mode allocate_fit_mode)
{
    size_t allocator_size = space_size + get_allocator_meta_size();

    try
    {
        if (parent_allocator) _trusted_memory = parent_allocator->allocate(allocator_size, 1);
        else _trusted_memory = ::operator new(allocator_size);
    }
    catch(const std::bad_alloc& e)
    {
        error_with_guard(get_typename() + "Cannot allocate\n");
        throw e;
    }

    auto ptr = reinterpret_cast<unsigned char *>(_trusted_memory);
	*reinterpret_cast<class logger **>(ptr) = logger;
	ptr += sizeof(class logger *);
	*reinterpret_cast<allocator **>(ptr) = parent_allocator;
	ptr += sizeof(allocator *);
	*reinterpret_cast<allocator_with_fit_mode::fit_mode *>(ptr) = allocate_fit_mode;
	ptr += sizeof(allocator_with_fit_mode::fit_mode);
	*reinterpret_cast<size_t *>(ptr) = space_size;
	ptr += sizeof(size_t);
	auto mutex = reinterpret_cast<std::mutex *>(ptr);
	construct(mutex);
	ptr += sizeof(std::mutex);
	auto first_forward = reinterpret_cast<void **>(ptr);
	*first_forward = reinterpret_cast<unsigned char *>(_trusted_memory) + get_allocator_meta_size();
	ptr += sizeof(void *);
	auto first_free_block = reinterpret_cast<void*>(ptr);

    get_byte_occupied_color(first_free_block).is_occupied = false;
	get_byte_occupied_color(first_free_block)._color = color::black;
	get_back(first_free_block) = nullptr;
	get_next(first_free_block) = nullptr;
	get_parent(first_free_block) = nullptr;
	get_left(first_free_block) = nullptr;
	get_right(first_free_block) = nullptr;

    debug_with_guard(get_typename() + "Constructor finished\n");
}

void * allocator_red_black_tree::get_first_fit(size_t size) const noexcept
{
	void * result = *get_first_block(_trusted_memory);
	while (result)
	{
		if (get_block_size(result, _trusted_memory) >= size) return result;
		result = get_right(result);
	}
	return result;
}

void * allocator_red_black_tree::get_best_fit(size_t size) const noexcept
{
	void * result = nullptr;
	void * node = *get_first_block(_trusted_memory);
	while (node)
	{
		size_t size_of_node = get_block_size(node, _trusted_memory);
		if (size_of_node >= size) result = node;
		if (size_of_node > size) node = get_left(node);
		else if(size_of_node < size) node = get_right(node);
		else node = nullptr;
	}
	return result;
}

void* allocator_red_black_tree::get_worst_fit(size_t size) const noexcept
{
	void * result = nullptr;
	void * node = *get_first_block(_trusted_memory);
	while (node)
	{
		if( get_block_size(node, _trusted_memory) >= size) result = node;
		node = get_right(node);
	}
	return result;
}

[[nodiscard]] void *allocator_red_black_tree::allocate(
    size_t value_size,
    size_t values_count)
{
    std::lock_guard<std::mutex> lock(get_mutex());

    debug_with_guard(get_typename() + "Allocate function started\n");

    size_t requested_memory = value_size * values_count;

    void * target;

    switch (get_fit_mode())
    {
        case allocator_with_fit_mode::fit_mode::first_fit:
			target = get_first_fit(requested_memory);
			break;
		case allocator_with_fit_mode::fit_mode::the_best_fit:
			target = get_best_fit(requested_memory);
			break;
		case allocator_with_fit_mode::fit_mode::the_worst_fit:
			target = get_worst_fit(requested_memory);
			break;
    }

    if (!target)
    {
        error_with_guard(get_typename() + "No suitable blocks found\n");
        throw std::bad_alloc();
    }

    remove_block(target);

    get_byte_occupied_color(target).is_occupied = true;
    get_parent(target) = _trusted_memory;
    size_t block_size = get_block_size(target, _trusted_memory);

    if (block_size < requested_memory + get_free_block_meta_size())
	{
		requested_memory = block_size;
		warning_with_guard(get_typename() + "Requested size has been changed\n");
	}
    else
    {
        void* new_target = reinterpret_cast<unsigned char*>(target) + get_occupied_block_meta_size() + requested_memory;

		get_next(new_target) = get_next(target);
		get_back(new_target) = target;
		get_next(target) = new_target;

		if (get_next(new_target)) get_back(get_next(new_target)) = new_target;
		get_byte_occupied_color(new_target).is_occupied = false;
		get_parent(new_target) = nullptr;

		insert_block(new_target);
    }

    debug_with_guard(get_typename() + "Allocate function finished\n");

    information_with_guard(get_typename() + "Blocks state: " + get_blocks_info_to_string(get_blocks_info()));

	return reinterpret_cast<unsigned char*>(target) + get_occupied_block_meta_size();
}

void allocator_red_black_tree::deallocate(
    void *at)
{
    std::lock_guard<std::mutex> lock(get_mutex());

	debug_with_guard(get_typename() + "Deallocate function started");

	void * block_ptr = reinterpret_cast<unsigned char*>(at) - get_occupied_block_meta_size();

	if (get_parent(block_ptr) != _trusted_memory)
	{
		error_with_guard("Invalid allocator\n");
		throw std::logic_error("Invalid allocator");
	}

	debug_with_guard("Block dump: " + get_dump(reinterpret_cast<char*>(at), get_block_size(block_ptr, _trusted_memory)));

	get_byte_occupied_color(block_ptr).is_occupied = false;

	if (get_back(block_ptr) && !get_byte_occupied_color(get_back(block_ptr)).is_occupied)
	{
		void* tmp = block_ptr;
		block_ptr = get_back(block_ptr);
		remove_block(block_ptr);
		get_next(block_ptr) = get_next(tmp);
		if (get_next(block_ptr)) get_back(get_next(block_ptr)) = block_ptr;
	}

	if (get_next(block_ptr) && !get_byte_occupied_color(get_next(block_ptr)).is_occupied)
	{
		void* tmp = get_next(block_ptr);
		remove_block(tmp);
		get_next(block_ptr) = get_next(tmp);
		if (get_next(block_ptr)) get_back(get_next(block_ptr)) = block_ptr;
	}


	insert_block(block_ptr);

	debug_with_guard(get_typename() + "Deallocate function finished");
	information_with_guard("Available_memory: " + std::to_string(get_free_size()));
	information_with_guard(get_typename() + " Blocks state: " + get_blocks_info_to_string(get_blocks_info()));
}

inline void allocator_red_black_tree::set_fit_mode(
		allocator_with_fit_mode::fit_mode mode)
{
	std::lock_guard<std::mutex> lock(get_mutex());
	auto byte_ptr = reinterpret_cast<unsigned char *>(_trusted_memory);
	byte_ptr += sizeof(logger*) + sizeof(allocator*);
	*reinterpret_cast<allocator_with_fit_mode::fit_mode*>(byte_ptr) = mode;
}

std::string allocator_red_black_tree::get_dump(char * at, size_t size)
{
	std::string result;
	for (auto i = 0; i < size; ++i) result += std::to_string(static_cast<int>(at[i])) + " ";
	return result;
}

inline allocator *allocator_red_black_tree::get_allocator() const
{
    auto ptr = reinterpret_cast<unsigned char *>(_trusted_memory);
	ptr += sizeof(logger *);
	return *reinterpret_cast<allocator **>(ptr);
}

inline std::mutex& allocator_red_black_tree::get_mutex() const noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(_trusted_memory);
	ptr += sizeof(logger *) + sizeof(allocator *)  + sizeof(fit_mode) + sizeof(size_t);
	return *reinterpret_cast<std::mutex *>(ptr);
}

inline logger *allocator_red_black_tree::get_logger() const
{
    return *reinterpret_cast<logger **>(_trusted_memory);
}

inline allocator_with_fit_mode::fit_mode& allocator_red_black_tree::get_fit_mode() const noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(_trusted_memory);
	ptr += sizeof(logger *) + sizeof(allocator *);
	return *reinterpret_cast<allocator_with_fit_mode::fit_mode *>(ptr);
}

allocator_red_black_tree::byte_occupied_color& allocator_red_black_tree::get_byte_occupied_color(void * current_block) noexcept
{
	return *reinterpret_cast<byte_occupied_color *>(current_block);
}

inline std::string allocator_red_black_tree::get_typename() const noexcept
{
    return rb_allocator;
}

inline size_t allocator_red_black_tree::get_allocator_meta_size() noexcept
{
	return sizeof(allocator *) + sizeof(logger *) + sizeof(allocator_with_fit_mode::fit_mode) + sizeof(std::mutex) + sizeof(block_size_t) + sizeof(block_pointer_t); // root
}

inline size_t allocator_red_black_tree::get_free_block_meta_size() noexcept
{
    // назад, вперед, родитель, левое поддерево, правое поддерева
	return sizeof(void*) * 5 + sizeof(byte_occupied_color);
}

inline size_t allocator_red_black_tree::get_occupied_block_meta_size() noexcept
{
    // назад, вперед, родитель
	return sizeof(void*) * 3 + sizeof(byte_occupied_color); // back* forward* parent* + 1 byte
}

inline size_t allocator_red_black_tree::get_block_size(void * current_block, void * trusted_memory) noexcept
{
	if (!get_next(current_block)) return reinterpret_cast<unsigned char *>(trusted_memory) + get_allocator_meta_size() + get_full_size(trusted_memory) - reinterpret_cast<unsigned char *>(current_block) - get_occupied_block_meta_size();
	return reinterpret_cast<unsigned char *>(get_next(current_block)) - reinterpret_cast<unsigned char *>(current_block) - get_occupied_block_meta_size();
}

size_t allocator_red_black_tree::get_free_size() const noexcept
{
	size_t res = 0;
	for (auto it = this->begin(), end = this->end(); it != end; ++it) if (!it.is_occupied()) res += it.size();
	return res;
}

inline size_t allocator_red_black_tree::get_full_size(void * trusted_memory) noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(trusted_memory);
	ptr += sizeof(logger *) + sizeof(allocator *) + sizeof(fit_mode);
	return *reinterpret_cast<size_t *>(ptr);
}

void ** allocator_red_black_tree::get_first_block(void * trusted_memory) noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(trusted_memory);
	ptr += sizeof(logger *) + sizeof(allocator *) + sizeof(fit_mode) + sizeof(size_t) + sizeof(std::mutex);
	return reinterpret_cast<void **>(ptr);
}

void *& allocator_red_black_tree::get_back(void * current_block) noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(current_block);
	ptr += sizeof(byte_occupied_color);
	return *reinterpret_cast<void **>(ptr);
}

void *& allocator_red_black_tree::get_next(void * current_block) noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(current_block);
	ptr += sizeof(byte_occupied_color) + sizeof(void *);
	return *reinterpret_cast<void **>(ptr);
}

void *& allocator_red_black_tree::get_parent(void * current_block) noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(current_block);
	ptr += sizeof(byte_occupied_color) + 2 * sizeof(void *);
	return *reinterpret_cast<void **>(ptr);
}

void *& allocator_red_black_tree::get_left(void * current_block) noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(current_block);
	ptr += sizeof(byte_occupied_color) + 3 * sizeof(void* );
	return *reinterpret_cast<void **>(ptr);
}

void *& allocator_red_black_tree::get_right(void * current_block) noexcept
{
	auto ptr = reinterpret_cast<unsigned char *>(current_block);
	ptr += sizeof(byte_occupied_color) + 4 * sizeof(void *);

	return *reinterpret_cast<void **>(ptr);
}

void allocator_red_black_tree::update_parent(void * current_block, void * new_parent) noexcept
{
	if (!get_parent(current_block))
    {
        *get_first_block(_trusted_memory) = new_parent;
        return;
    }
    if (current_block == get_left(get_parent(current_block))) get_left(get_parent(current_block)) = new_parent;
    else get_right(get_parent(current_block)) = new_parent;
}

std::vector<allocator_test_utils::block_info> allocator_red_black_tree::get_blocks_info() const noexcept
{
	std::vector<allocator_test_utils::block_info> result;

	for (auto it = this->begin(), end = this->end(); it != end; ++it) result.push_back({it.size(), it.is_occupied()});

	return result;

}

std::string allocator_red_black_tree::get_blocks_info_to_string(const std::vector<allocator_test_utils::block_info>& vector) const noexcept
{
	std::string str = "";
	for (auto& it : vector)
	{
		if (it.is_block_occupied) str += "<occup>";
		else str += "<avail>";

		str += "<" + std::to_string(it.block_size) + ">, ";
	}
	return str;
}

// iterator implement start


allocator_red_black_tree::iterator::iterator() : _ptr(nullptr), _trusted_memory(nullptr) {}

allocator_red_black_tree::iterator::iterator(void * memory)
{
	_ptr = reinterpret_cast<void*>(reinterpret_cast<unsigned char*>(memory) + get_allocator_meta_size());
	_trusted_memory = memory;
}

allocator_red_black_tree::iterator allocator_red_black_tree::begin() const noexcept
{
	return {_trusted_memory};
}

allocator_red_black_tree::iterator allocator_red_black_tree::end() const noexcept
{
	return {};
}

bool allocator_red_black_tree::iterator::operator==(const allocator_red_black_tree::iterator &other) const noexcept
{
	return _ptr == other._ptr;
}

bool allocator_red_black_tree::iterator::operator!=(const allocator_red_black_tree::iterator &other) const noexcept
{
	return !(*this == other);
}

allocator_red_black_tree::iterator& allocator_red_black_tree::iterator::operator++() noexcept
{
	_ptr = get_next(_ptr);
	return *this;
}

allocator_red_black_tree::iterator allocator_red_black_tree::iterator::operator++(int) noexcept
{
	auto tmp = *this;
	++(*this);
	return tmp;
}

size_t allocator_red_black_tree::iterator::size()
{
	return get_block_size(_ptr, _trusted_memory);
}

void* allocator_red_black_tree::iterator::find_free_block() const noexcept
{
	return _ptr;
}

bool allocator_red_black_tree::iterator::is_occupied() const noexcept
{
	return get_byte_occupied_color(_ptr).is_occupied;
}


// iterator implement end


// red black tree functions implement start


void allocator_red_black_tree::remove_block(void * current_block) noexcept
{
	void * parent;
	bool need_rebalance = false;

	if (!get_right(current_block) && !get_left(current_block))
	{
		parent = get_parent(current_block);
		update_parent(current_block, nullptr);
		need_rebalance = get_byte_occupied_color(current_block)._color == color::black;
	}
	else if (!get_right(current_block) || !get_left(current_block))
	{
		void * button_node = get_right(current_block) ? get_right(current_block) : get_left(current_block);
		get_byte_occupied_color(button_node)._color = color::black;
		update_parent(current_block, button_node);
		get_parent(button_node) = get_parent(current_block);
	}
	else
	{
		void * change_node = get_left(current_block);

		while (get_right(change_node)) change_node = get_right(change_node);

		need_rebalance = get_left(change_node) == nullptr && get_byte_occupied_color(change_node)._color == color::black;

		parent = get_parent(change_node);

		if (get_byte_occupied_color(change_node)._color == color::black && get_left(change_node) != nullptr)
		    get_byte_occupied_color(get_left(change_node))._color = color::black;

		update_parent(current_block, change_node);
		get_right(change_node) = get_right(current_block);
		get_parent(get_right(change_node)) = change_node;

		if (get_parent(change_node) != current_block)
		{
			update_parent(change_node, get_left(change_node));
			if (get_left(change_node)) get_parent(get_left(change_node)) = get_parent(change_node);
			get_left(change_node) = get_left(current_block);
			get_parent(get_left(change_node)) = change_node;
		}
		else parent = change_node;

		get_byte_occupied_color(change_node)._color = get_byte_occupied_color(current_block)._color;
		get_parent(change_node) = get_parent(current_block);

	}

	if (need_rebalance) rebalance(parent);
}

void allocator_red_black_tree::rebalance(void * parent, void * deleted_block)
{
	if (!parent)
	{
		if (deleted_block) get_byte_occupied_color(deleted_block)._color = color::black;
        return;
	}

    bool is_left = deleted_block == get_left(parent);

    void* brother = is_left ? get_right(parent) : get_left(parent);

    if (get_byte_occupied_color(brother)._color == color::red)
    {
        is_left ? small_left_rotate(parent) : small_right_rotate(parent);

        get_byte_occupied_color(parent)._color = color::red;
        get_byte_occupied_color(brother)._color = color::black;

        rebalance(parent, deleted_block);
        return;
    }

    void * far_cousin = is_left ? get_right(brother) : get_left(brother);
    void * near_cousin = is_left ? get_left(brother) : get_right(brother);

    if (far_cousin && get_byte_occupied_color(far_cousin)._color == color::red)
    {
        is_left ? small_left_rotate(parent) : small_right_rotate(parent);

        get_byte_occupied_color(brother)._color = get_byte_occupied_color(parent)._color;
        get_byte_occupied_color(parent)._color = color::black;
        get_byte_occupied_color(far_cousin)._color = color::black;
        return;
    }
    if (near_cousin && get_byte_occupied_color(near_cousin)._color == color::red)
    {
        is_left ? big_left_rotate(parent) : big_right_rotate(parent);

        get_byte_occupied_color(near_cousin)._color = get_byte_occupied_color(parent)._color;
        get_byte_occupied_color(parent)._color = color::black;
        return;
    }
    get_byte_occupied_color(brother)._color = color::red;

    if (get_byte_occupied_color(parent)._color == color::red) get_byte_occupied_color(parent)._color = color::black;
    else rebalance(get_parent(parent), parent);
}

void allocator_red_black_tree::small_right_rotate(void* root) noexcept
{
	if (!get_left(root)) return;

    void* left_son = get_left(root);
    update_parent(root, left_son);
    get_parent(left_son) = get_parent(root);
    void * right_son_of_left_son_of_joint = get_right(left_son);
    get_right(left_son) = root;
    get_parent(root) = left_son;
    get_left(root) = right_son_of_left_son_of_joint;
    if (right_son_of_left_son_of_joint) get_parent(right_son_of_left_son_of_joint) = root;
}

void allocator_red_black_tree::small_left_rotate(void* root) noexcept
{
	if (!get_right(root)) return;

    void * right_son = get_right(root);
    update_parent(root, right_son);
    get_parent(right_son) = get_parent(root);
    void * left_son_of_right_son_of_joint = get_left(right_son);
    get_left(right_son) = root;
    get_parent(root) = right_son;
    get_right(root) = left_son_of_right_son_of_joint;
    if (left_son_of_right_son_of_joint) get_parent(left_son_of_right_son_of_joint) = root;
}

void allocator_red_black_tree::big_right_rotate(void* root) noexcept
{
	if (get_left(root) && get_right(get_left(root)))
	{
		void* node = get_left(root);
		small_left_rotate(node);
		small_right_rotate(root);
	}
}

void allocator_red_black_tree::big_left_rotate(void *root) noexcept
{
	if (get_right(root) && get_left(get_right(root)))
	{
		void* node = get_right(root);
		small_right_rotate(node);
		small_left_rotate(root);
	}
}

void allocator_red_black_tree::insert_block(void * current_block) noexcept
{
	void* root = *get_first_block(_trusted_memory);
	void* parent = nullptr;

	while (root)
	{
		if (get_block_size(current_block, _trusted_memory) < get_block_size(root, _trusted_memory))
		{
			parent = root;
			root = get_left(root);
		}
		else
		{
			parent = root;
			root = get_right(root);
		}
	}

	get_parent(current_block) = parent;
	get_left(current_block) = nullptr;
	get_right(current_block) = nullptr;

	get_byte_occupied_color(current_block).is_occupied = false;
	get_byte_occupied_color(current_block)._color = color::red;

	if (!parent) *get_first_block(_trusted_memory) = current_block;
	else
	{
		if(get_block_size(current_block, _trusted_memory) >= get_block_size(parent, _trusted_memory))
            get_right(parent) = current_block;
		else get_left(parent) = current_block;
	}

	if (!parent)
	{
		get_byte_occupied_color(current_block)._color = color::black;
		return;
	}

	bool is_left = current_block == get_left(parent);

	while (true)
	{
		if (get_byte_occupied_color(parent)._color == color::black) break;
		if (get_byte_occupied_color(parent)._color == color::red)
		{
			void * grand = get_parent(parent);

			if (get_right(grand) == parent)
			{
				if ((!get_left(grand) || get_byte_occupied_color(get_left(grand))._color == color::black) && !is_left)
				{
					get_byte_occupied_color(grand)._color = color::red;
					small_left_rotate(grand);
					get_byte_occupied_color(get_parent(grand))._color = color::black;
					break;
				}
				else if ((!get_left(grand) || get_byte_occupied_color(get_left(grand))._color == color::black) && is_left)
				{
					get_byte_occupied_color(grand)._color = color::red;
					big_left_rotate(grand);
					get_byte_occupied_color(get_parent(grand))._color = color::black;
					break;
				}
				else if (get_byte_occupied_color(get_left(grand))._color == color::red)
				{
					get_byte_occupied_color(grand)._color = color::red;
					get_byte_occupied_color(get_left(grand))._color = color::black;
					get_byte_occupied_color(get_right(grand))._color = color::black;
					current_block = grand;
				}
			}
			else if (get_left(grand) == parent)
			{
				if ((!get_right(grand) || get_byte_occupied_color(get_right(grand))._color == color::black) && is_left)
				{
					get_byte_occupied_color(grand)._color = color::red;
					small_right_rotate(grand);
					get_byte_occupied_color(get_parent(grand))._color = color::black;
					break;
				}
				else if((!get_right(grand) || get_byte_occupied_color(get_right(grand))._color == color::black) && !is_left)
				{
					get_byte_occupied_color(grand)._color = color::red;
					big_right_rotate(grand);
					get_byte_occupied_color(get_parent(grand))._color = color::black;
					break;
				}
				else if(get_byte_occupied_color(get_right(grand))._color == color::red)
				{
					get_byte_occupied_color(grand)._color = color::red;
					get_byte_occupied_color(get_left(grand))._color = color::black;
					get_byte_occupied_color(get_right(grand))._color = color::black;
					current_block = grand;
				}
			}
		}

		if (!get_parent(current_block))
		{
			get_byte_occupied_color(current_block)._color = color::black;
			break;
		}
		parent = get_parent(current_block);
		is_left = current_block == get_left(parent);
	}
}


// red black tree functions implement end
