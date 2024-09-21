#ifndef MATH_PRACTICE_AND_OPERATING_SYSTEMS_BINARY_SEARCH_TREE_H
#define MATH_PRACTICE_AND_OPERATING_SYSTEMS_BINARY_SEARCH_TREE_H

#include <list>
#include <queue>
#include <vector>
#include <logger.h>
#include <logger_guardant.h>
#include <allocator.h>
#include <allocator_guardant.h>
#include <not_implemented.h>
#include <search_tree.h>
#include <associative_container.h>
#include <stack>

template<typename tkey, typename tvalue>
class binary_search_tree:
    public search_tree<tkey, tvalue>
{

protected:
    
    struct node
    {
    
    public:
        
        tkey key;
        tvalue value;
        node *left_subtree;
        node *right_subtree;
    
    public:
        
        explicit node(tkey const &key, tvalue const &value);
        explicit node(tkey const &key, tvalue &&value);

        virtual unsigned char get_height() 
        {
            return 0;
        }

        virtual void set_height(unsigned char height)
        {}
    };

public:

    virtual size_t get_node_size() const
    {
        return sizeof(binary_search_tree<tkey, tvalue>::node);
    }

    virtual void call_node_constructor(typename binary_search_tree<tkey, tvalue>::node * raw_space, tkey const &key, tvalue const &value) const
    {
        allocator::construct(raw_space, key, value);
    }

    virtual void call_node_constructor(typename binary_search_tree<tkey, tvalue>::node * raw_space, tkey const &key, tvalue &&value) const
    {
        allocator::construct(raw_space, key, std::move(value));
    }


    // region iterators definition

    struct iterator_data
    {

    public:
        
        unsigned int depth;
        tkey key;
        tvalue value;
    
    private:

        bool _is_state_initialized;

    public:

        inline tkey const &get_key() const noexcept
        {
            if (is_state_initialized()) return key;
            throw std::logic_error("[BST] [Iterator Data] [Get Key] Iterator data is not initialized\n");
        }

        inline tvalue const &get_value() const noexcept
        {
            if (is_state_initialized()) return value;
            throw std::logic_error("[BST] [Iterator Data] [Get value] Iterator data is not initialized\n");
        }

        inline bool is_state_initialized() const noexcept
        {
            return _is_state_initialized;
        }
    
    public:

        explicit iterator_data():
            _is_state_initialized(false)
        {}

        explicit iterator_data(unsigned int depth, tkey const &key, tvalue const &value):
            depth(depth),
            key(key),
            value(value),
            _is_state_initialized(true)
        {}

        explicit iterator_data(unsigned int depth, tkey const &key, tvalue &&value):
            depth(depth),
            key(key),
            value(std::move(value)),
            _is_state_initialized(true)
        {}
    };

public:

    virtual void call_iterator_data_construct(binary_search_tree<tkey, tvalue>::iterator_data * raw_space, binary_search_tree<tkey, tvalue>::node * subtree_root, unsigned int depth) const
    {
        allocator::construct(raw_space, depth, subtree_root->key, subtree_root->value);
    }

    virtual size_t get_iterator_data_size() const noexcept
    {
        return sizeof(binary_search_tree<tkey, tvalue>::iterator_data);
    }

    iterator_data * create_iterator_data(unsigned int depth, node *& ptr) const
    {
        this->trace_with_guard("[BST] Creating iterator data...\n");
        iterator_data * data = nullptr;
        try
        {
            data = reinterpret_cast<iterator_data*>(this->allocate_with_guard(get_iterator_data_size(), 1));
            call_iterator_data_construct(data, ptr, depth);
        }
        catch(const std::exception& e)
        {
            this->error_with_guard("[BST] BadAlloc corrupted while creating iterator data\n");
            throw e;
        }
        this->trace_with_guard("[BST] Iterator data was successfully created\n");
        return data;
    }

    void delete_iterator_data(iterator_data * data) const
    {
        allocator::destruct(data);
        this->deallocate_with_guard(data);
    }

protected:

    void clear(node *&subtree_root)
    {
        if (subtree_root == nullptr) return;
        clear(subtree_root->left_subtree);
        clear(subtree_root->right_subtree);
        subtree_root->~node();
        this->deallocate_with_guard(subtree_root);
        subtree_root = nullptr;
        this->trace_with_guard("Node was deleted\n");
    }

    node *copy(node const *subtree_root)
    {
        if (subtree_root == nullptr) return nullptr;

        node *subtree_root_copied = reinterpret_cast<node *>(this->allocate_with_guard(sizeof(node), 1));
        allocator::construct(subtree_root_copied, subtree_root -> key, subtree_root -> value);
        subtree_root_copied->left_subtree = copy(subtree_root->left_subtree);
        subtree_root_copied->right_subtree = copy(subtree_root->right_subtree);

        this->trace_with_guard("Node was copied\n");
        return subtree_root_copied;
    }

protected:

    void find_prefix_path(unsigned int depth, std::queue<typename binary_search_tree<tkey, tvalue>::iterator_data*> &way, binary_search_tree<tkey, tvalue>::node * ptr) const
    {
        if (ptr == nullptr) return;
        way.push(this -> create_iterator_data(depth, ptr));
        if (ptr -> left_subtree != nullptr) find_prefix_path(depth + 1, way, ptr -> left_subtree);
        if (ptr -> right_subtree != nullptr) find_prefix_path(depth + 1, way, ptr -> right_subtree);
    }

    void find_infix_path(unsigned int depth, std::queue<typename binary_search_tree<tkey, tvalue>::iterator_data*> &way, binary_search_tree<tkey, tvalue>::node * ptr) const
    {
        if (ptr == nullptr) return;
        if (ptr -> left_subtree != nullptr) find_infix_path(depth + 1, way, ptr -> left_subtree);
        way.push(this -> create_iterator_data(depth, ptr));
        if (ptr -> right_subtree != nullptr) find_infix_path(depth + 1, way, ptr -> right_subtree);
    }

    void find_postfix_path(unsigned int depth, std::queue<typename binary_search_tree<tkey, tvalue>::iterator_data*> &way, binary_search_tree<tkey, tvalue>::node * ptr) const
    {
        if (ptr == nullptr) return;
        if (ptr -> left_subtree != nullptr) find_postfix_path(depth + 1, way, ptr -> left_subtree);
        if (ptr -> right_subtree != nullptr) find_postfix_path(depth + 1, way, ptr -> right_subtree);
        way.push(this -> create_iterator_data(depth, ptr));
    }

    void clear_way(std::queue<typename binary_search_tree<tkey, tvalue>::iterator_data*> &way) const
    {
        iterator_data * data;
        while (!way.empty())
        {
            data = way.front();
            way.pop();
            allocator::destruct(data);
            this->deallocate_with_guard(data);
        }
    }

public:

    std::queue<iterator_data *> copy_way(std::queue<iterator_data *> way)
    {
        iterator_data * data;
        std::queue<iterator_data *> new_way;
        while (!way.empty())
        {
            data = way.front();
            way.pop();
            new_way.push(create_iterator_data(data->depth, data->key, data->value));
        }
        return new_way;
    }

    void iterators_copy(iterator_data *& ptr, iterator_data *& other_ptr, 
    binary_search_tree<tkey, tvalue> const *& tree, binary_search_tree<tkey, tvalue> const *& other_tree, 
    std::queue<iterator_data *> &way, std::queue<iterator_data*> &other_way) const
    {
        if (!way.empty()) tree->clear_way(way);
        ptr = nullptr;
        tree = nullptr;

        tree = other_tree;
        way = this->copy_way(other_way);
        ptr = way.front();
    }

    void iterators_move(iterator_data *& ptr, iterator_data *& other_ptr, 
    binary_search_tree<tkey, tvalue> const *& tree, binary_search_tree<tkey, tvalue> const *& other_tree, 
    std::queue<iterator_data *> &way, std::queue<iterator_data*> &other_way) const
    {
        if (!way.empty()) tree->clear_way(way);
        ptr = nullptr;
        tree = nullptr;

        tree = other_tree;
        ptr = other_ptr;

        way = other_way;
        while(!other_way.empty()) way.pop();
    }

public:
    
    class prefix_iterator final
    {

    public:
        
        explicit prefix_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(prefix_iterator const &other) const noexcept;
        
        bool operator!=(prefix_iterator const &other) const noexcept;
        
        prefix_iterator &operator++();
        
        prefix_iterator const operator++(int not_used);
        
        iterator_data *operator*() const;

        // правило пяти, чтобы почистить путь

        ~prefix_iterator()
        {
            _tree->clear_way(_way);
        }

        prefix_iterator(prefix_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_iterator(prefix_iterator && other) noexcept
        {
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_iterator &operator=(prefix_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_iterator &operator=(prefix_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;

    };
    
    class prefix_const_iterator final
    {
    
    public:
        
        explicit prefix_const_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(prefix_const_iterator const &other) const noexcept;
        
        bool operator!=(prefix_const_iterator const &other) const noexcept;
        
        prefix_const_iterator &operator++();
        
        prefix_const_iterator const operator++(int not_used);
        
        iterator_data const *operator*() const;

        ~prefix_const_iterator()
        {
            _tree->clear_way(_way);
        }

        prefix_const_iterator(prefix_const_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_const_iterator(prefix_const_iterator && other) noexcept
        {
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_const_iterator &operator=(prefix_const_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_const_iterator &operator=(prefix_const_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
    };
    
    class prefix_reverse_iterator final: protected allocator_guardant
    {
    
    public:
        
        explicit prefix_reverse_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(prefix_reverse_iterator const &other) const noexcept;
        
        bool operator!=(prefix_reverse_iterator const &other) const noexcept;
        
        prefix_reverse_iterator &operator++();
        
        prefix_reverse_iterator const operator++(int not_used);
        
        iterator_data *operator*() const;

        ~prefix_reverse_iterator()
        {
            _tree->clear_way(_way);
        }

        prefix_reverse_iterator(prefix_reverse_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_reverse_iterator(prefix_reverse_iterator && other) noexcept
        {
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_reverse_iterator &operator=(prefix_reverse_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_reverse_iterator &operator=(prefix_reverse_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class prefix_const_reverse_iterator final
    {
    
    public:
        
        explicit prefix_const_reverse_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(prefix_const_reverse_iterator const &other) const noexcept;
        
        bool operator!=(prefix_const_reverse_iterator const &other) const noexcept;
        
        prefix_const_reverse_iterator &operator++();
        
        prefix_const_reverse_iterator const operator++(int not_used);
        
        iterator_data const *operator*() const;

        ~prefix_const_reverse_iterator()
        {
            _tree->clear_way(_way);
        }

        prefix_const_reverse_iterator(prefix_const_reverse_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_const_reverse_iterator(prefix_const_reverse_iterator && other) noexcept
        {
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_const_reverse_iterator &operator=(prefix_const_reverse_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        prefix_const_reverse_iterator &operator=(prefix_const_reverse_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class infix_iterator final
    {

    public:

        explicit infix_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(infix_iterator const &other) const noexcept;
        
        bool operator!=(infix_iterator const &other) const noexcept;
        
        infix_iterator &operator++();
        
        infix_iterator const operator++(int not_used);
        
        iterator_data *operator*() const;

        ~infix_iterator()
        {
            _tree->clear_way(_way);
        }

        infix_iterator(infix_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_iterator(infix_iterator && other) noexcept
        {
            if (!_way.empty()) _tree->clear_way(_way);
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_iterator &operator=(infix_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_iterator &operator=(infix_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class infix_const_iterator final
    {
    
    public:
        
        explicit infix_const_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(infix_const_iterator const &other) const noexcept;
        
        bool operator!=(infix_const_iterator const &other) const noexcept;
        
        infix_const_iterator &operator++();
        
        infix_const_iterator const operator++(int not_used);
        
        iterator_data const *operator*() const;

        ~infix_const_iterator()
        {
            _tree->clear_way(_way);
        }

        infix_const_iterator(infix_const_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_const_iterator(infix_const_iterator && other) noexcept
        {
            if (!_way.empty()) _tree->clear_way(_way);
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_const_iterator &operator=(infix_const_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_const_iterator &operator=(infix_const_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class infix_reverse_iterator final
    {
    
    public:
        
        explicit infix_reverse_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(infix_reverse_iterator const &other) const noexcept;
        
        bool operator!=(infix_reverse_iterator const &other) const noexcept;
        
        infix_reverse_iterator &operator++();
        
        infix_reverse_iterator const operator++(int not_used);
        
        iterator_data *operator*() const;

        ~infix_reverse_iterator()
        {
            _tree->clear_way(_way);
        }

        infix_reverse_iterator(infix_reverse_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_reverse_iterator(infix_reverse_iterator && other) noexcept
        {
            if (!_way.empty()) _tree->clear_way(_way);
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_reverse_iterator &operator=(infix_reverse_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_reverse_iterator &operator=(infix_reverse_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
    };
    
    class infix_const_reverse_iterator final
    {
    
    public:
        
        explicit infix_const_reverse_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(infix_const_reverse_iterator const &other) const noexcept;
        
        bool operator!=(infix_const_reverse_iterator const &other) const noexcept;
        
        infix_const_reverse_iterator &operator++();
        
        infix_const_reverse_iterator const operator++(int not_used);
        
        iterator_data const *operator*() const;

        ~infix_const_reverse_iterator()
        {
            _tree->clear_way(_way);
        }

        infix_const_reverse_iterator(infix_const_reverse_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_const_reverse_iterator(infix_const_reverse_iterator && other) noexcept
        {
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_const_reverse_iterator &operator=(infix_const_reverse_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        infix_const_reverse_iterator &operator=(infix_const_reverse_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class postfix_iterator final
    {
    
    public:
        
        explicit postfix_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(postfix_iterator const &other) const noexcept;
        
        bool operator!=(postfix_iterator const &other) const noexcept;
        
        postfix_iterator &operator++();
        
        postfix_iterator const operator++(int not_used);
        
        iterator_data *operator*() const;

        ~postfix_iterator()
        {
            _tree->clear_way(_way);
        }

        postfix_iterator(postfix_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_iterator(postfix_iterator && other) noexcept
        {
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_iterator &operator=(postfix_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_iterator &operator=(postfix_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class postfix_const_iterator final
    {
    
    public:
        
        explicit postfix_const_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(postfix_const_iterator const &other) const noexcept;
        
        bool operator!=(postfix_const_iterator const &other) const noexcept;
        
        postfix_const_iterator &operator++();
        
        postfix_const_iterator const operator++(int not_used);
        
        iterator_data const *operator*() const;

        ~postfix_const_iterator()
        {
            _tree->clear_way(_way);
        }

        postfix_const_iterator(postfix_const_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_const_iterator(postfix_const_iterator && other) noexcept
        {
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_const_iterator &operator=(postfix_const_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_const_iterator &operator=(postfix_const_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class postfix_reverse_iterator final
    {
    
    public:
        
        explicit postfix_reverse_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(postfix_reverse_iterator const &other) const noexcept;
        
        bool operator!=(postfix_reverse_iterator const &other) const noexcept;
        
        postfix_reverse_iterator &operator++();
        
        postfix_reverse_iterator const operator++(int not_used);
        
        iterator_data *operator*() const;

        ~postfix_reverse_iterator()
        {
            _tree->clear_way(_way);
        }

        postfix_reverse_iterator(postfix_reverse_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_reverse_iterator(postfix_reverse_iterator && other) noexcept
        {
            if (!_way.empty()) _tree->clear_way(_way);
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_reverse_iterator &operator=(postfix_reverse_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_reverse_iterator &operator=(postfix_reverse_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    class postfix_const_reverse_iterator final
    {
    
    public:
        
        explicit postfix_const_reverse_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root);
    
    public:
        
        bool operator==(postfix_const_reverse_iterator const &other) const noexcept;
        
        bool operator!=(postfix_const_reverse_iterator const &other) const noexcept;
        
        postfix_const_reverse_iterator &operator++();
        
        postfix_const_reverse_iterator const operator++(int not_used);
        
        iterator_data const *operator*() const;

        ~postfix_const_reverse_iterator()
        {
            _tree->clear_way(_way);
        }

        postfix_const_reverse_iterator(postfix_const_reverse_iterator const & other)
        {
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_const_reverse_iterator(postfix_const_reverse_iterator && other) noexcept
        {
            if (!_way.empty()) _tree->clear_way(_way);
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_const_reverse_iterator &operator=(postfix_const_reverse_iterator const & other)
        {
            if (this == &other) return *this;
            other._tree->iterators_copy(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

        postfix_const_reverse_iterator &operator=(postfix_const_reverse_iterator && other) noexcept
        {
            if (this == &other) return *this;
            other._tree->iterators_move(_ptr, other._ptr, _tree, other._tree, _way, other._way);
        }

    private:

        std::queue<iterator_data *> _way;

        iterator_data * _ptr;

        const binary_search_tree<tkey, tvalue> * _tree;
        
    };
    
    // endregion iterators definition

protected:
    
    // region target operations strategies definition
    
    enum class insertion_of_existent_key_attempt_strategy
    {
        update_value,
        throw_an_exception
    };
    
    enum class disposal_of_nonexistent_key_attempt_strategy
    {
        do_nothing,
        throw_an_exception
    };
    
    // endregion target operations strategies definition
    
    // region target operations associated exception types
    
    class insertion_of_existent_key_attempt_exception final:
        public std::logic_error
    {
    
    private:
        
        tkey _key;
    
    public:
        
        explicit insertion_of_existent_key_attempt_exception(tkey const &key);
        
    public:
        
        tkey const &get_key() const noexcept;
    
    };
    
    class obtaining_of_nonexistent_key_attempt_exception final:
        public std::logic_error
    {
    
    private:
        
        tkey _key;
        
    public:
        
        explicit obtaining_of_nonexistent_key_attempt_exception(tkey const &key);
        
    public:
        
        tkey const &get_key() const noexcept;
        
    };
    
    class disposal_of_nonexistent_key_attempt_exception final:
        public std::logic_error
    {
    
    private:
        
        tkey _key;
    
    public:
        
        explicit disposal_of_nonexistent_key_attempt_exception(tkey const &key);
        
    public:
        
        tkey const &get_key() const noexcept;
    
    };
    
    // endregion target operations associated exception types
        
    // region template methods definition
    
    class template_method_basics:
        public logger_guardant
    {
    
    public:
    
        binary_search_tree<tkey, tvalue> *_tree;
        
    public:
    
        explicit template_method_basics(binary_search_tree<tkey, tvalue> *tree);

    protected:

        virtual void balance(std::stack<node **> &path) {}

        std::stack<node **> find_path(tkey const &key) const
        {
            std::stack<node **> result_path;

            node **path_finder = &(_tree->_root);
            auto const &comparer = _tree->_keys_comparer;

            while (true)
            {
                result_path.push(path_finder);

                if (*path_finder == nullptr) break;

                auto comparison_result = comparer(key, (*path_finder)->key);
                if (comparison_result == 0) break;

                path_finder = comparison_result < 0
                    ? &((*path_finder)->left_subtree)
                    : &((*path_finder)->right_subtree);
            }

            return result_path;
        }

    protected:
    
        [[nodiscard]] logger *get_logger() const noexcept final;
        
    };
    
    class insertion_template_method:
        public template_method_basics,
        public allocator_guardant
    {

    public:

        binary_search_tree<tkey, tvalue> *_tree;

    public:

        binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy _insertion_strategy;

    public:
        
        explicit insertion_template_method(
            binary_search_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy);
        
    public:
        
        void insert(tkey const &key, tvalue const &value);
        
        void insert(tkey const &key, tvalue &&value);
    
        void set_insertion_strategy(
            typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy) noexcept;
    
    protected:
        
        [[nodiscard]] allocator *get_allocator() const noexcept final;
        
    };
    
    class obtaining_template_method:
        public template_method_basics
    {
    
    public:

        binary_search_tree<tkey, tvalue> *_tree;
    
    public:
        
        explicit obtaining_template_method(binary_search_tree<tkey, tvalue> *tree);
    
    public:
        
        tvalue const &obtain(tkey const &key);

        std::vector<typename associative_container<tkey, tvalue>::key_value_pair> obtain_between(
            tkey const &lower_bound,
            tkey const &upper_bound,
            bool lower_bound_inclusive,
            bool upper_bound_inclusive)
        {
            std::vector<typename associative_container<tkey, tvalue>::key_value_pair> range;
            std::stack<node *> path;
            node *current = this->_tree->_root;
            
            while (true)
            {
                if (current == nullptr) break;

                auto comparison_result = this->_tree->_keys_comparer(lower_bound, current->key);
                path.push(current);

                if (comparison_result == 0)
                {
                    if (lower_bound_inclusive) break;

                    current = current->right_subtree;
                }
                else if (comparison_result < 0) current = current->left_subtree;
                else current = current->right_subtree;

                if (current == nullptr && this->_tree->_keys_comparer(path.top()->key, lower_bound) < (lower_bound_inclusive
                    ? 0
                    : 1))
                {
                    path = std::move(std::stack<node *>());
                }
            }

            auto it = infix_iterator(_tree, _tree->_root);
            while ((it != this->_tree->end_infix()) && (this->_tree->_keys_comparer(upper_bound, (*it)->get_key()) > (upper_bound_inclusive
                ? -1
                : 0)))
            {
                range.push_back(std::move(typename associative_container<tkey, tvalue>::key_value_pair((*it)->get_key(), (*it)->get_value())));
                ++it;
            }

            return range;
        }

    };
    
    class disposal_template_method:
        public template_method_basics,
        public allocator_guardant
    {
    
    public:

        binary_search_tree<tkey, tvalue> *_tree;

    public:

        binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy _disposal_strategy;
    
    public:
        
        explicit disposal_template_method(
            binary_search_tree<tkey, tvalue> *tree,
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy);
        
    public:
        
        tvalue dispose(tkey const &key);
        
        void set_disposal_strategy(
            typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) noexcept;
    
    protected:
        
        // TODO: think about it!
    
    protected:

        template<typename T>
        inline void swap(T &&one, T &&another)
        {
            T temp = std::move(one);
            one = std::move(another);
            another = std::move(temp);
        }
        
        [[nodiscard]] allocator *get_allocator() const noexcept final;
        
    };
    
    // endregion template methods definition

protected:
    
    explicit binary_search_tree(
        typename binary_search_tree<tkey, tvalue>::insertion_template_method *insertion_template,
        typename binary_search_tree<tkey, tvalue>::obtaining_template_method *obtaining_template,
        typename binary_search_tree<tkey, tvalue>::disposal_template_method *disposal_template,
        std::function<int(tkey const &, tkey const &)>,
        allocator *allocator,
        logger *logger);

public:
    
    explicit binary_search_tree(
        std::function<int(tkey const &, tkey const &)> comparer = std::less<tkey>(),
        allocator *allocator = nullptr,
        logger *logger = nullptr,
        typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy = binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy::throw_an_exception,
        typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy = binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy::throw_an_exception);

public:
    
    binary_search_tree(binary_search_tree<tkey, tvalue> const &other);
    
    binary_search_tree(binary_search_tree<tkey, tvalue> &&other) noexcept;
    
    binary_search_tree<tkey, tvalue> &operator=(binary_search_tree<tkey, tvalue> const &other);
    
    binary_search_tree<tkey, tvalue> &operator=(binary_search_tree<tkey, tvalue> &&other) noexcept;
    
    ~binary_search_tree() override;

public:
    
    void insert(tkey const &key, tvalue const &value) final;
    
    void insert(tkey const &key, tvalue &&value) final;
    
    tvalue const &obtain(tkey const &key) final;
    
    std::vector<typename associative_container<tkey, tvalue>::key_value_pair> obtain_between(
        tkey const &lower_bound,
        tkey const &upper_bound,
        bool lower_bound_inclusive,
        bool upper_bound_inclusive) final;
    
    tvalue dispose(tkey const &key) final;
    
public:
    
    void set_insertion_strategy(
        typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy) noexcept;
    
    void set_removal_strategy(
        typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) noexcept;

public:
    
    node *_root = nullptr;
    insertion_template_method *_insertion_template = nullptr;
    obtaining_template_method *_obtaining_template = nullptr;
    disposal_template_method *_disposal_template = nullptr;

public:
    
    // region iterators requests definition
    
    prefix_iterator begin_prefix() const noexcept;
    
    prefix_iterator end_prefix() const noexcept;
    
    prefix_const_iterator cbegin_prefix() const noexcept;
    
    prefix_const_iterator cend_prefix() const noexcept;
    
    prefix_reverse_iterator rbegin_prefix() const noexcept;
    
    prefix_reverse_iterator rend_prefix() const noexcept;
    
    prefix_const_reverse_iterator crbegin_prefix() const noexcept;
    
    prefix_const_reverse_iterator crend_prefix() const noexcept;
    
    infix_iterator begin_infix() const noexcept;
    
    infix_iterator end_infix() const noexcept;
    
    infix_const_iterator cbegin_infix() const noexcept;
    
    infix_const_iterator cend_infix() const noexcept;
    
    infix_reverse_iterator rbegin_infix() const noexcept;
    
    infix_reverse_iterator rend_infix() const noexcept;
    
    infix_const_reverse_iterator crbegin_infix() const noexcept;
    
    infix_const_reverse_iterator crend_infix() const noexcept;
    
    postfix_iterator begin_postfix() const noexcept;
    
    postfix_iterator end_postfix() const noexcept;
    
    postfix_const_iterator cbegin_postfix() const noexcept;
    
    postfix_const_iterator cend_postfix() const noexcept;
    
    postfix_reverse_iterator rbegin_postfix() const noexcept;
    
    postfix_reverse_iterator rend_postfix() const noexcept;
    
    postfix_const_reverse_iterator crbegin_postfix() const noexcept;
    
    postfix_const_reverse_iterator crend_postfix() const noexcept;
    
    // endregion iterators requests definition

public:
    
    // region subtree rotations definition
    
    void small_left_rotation(
        typename binary_search_tree<tkey, tvalue>::node *&subtree_root,
        bool validate = true) const;
    
    void small_right_rotation(
        typename binary_search_tree<tkey, tvalue>::node *&subtree_root,
        bool validate = true) const;
    
    void big_left_rotation(
        typename binary_search_tree<tkey, tvalue>::node *&subtree_root,
        bool validate = true) const;
    
    void big_right_rotation(
        typename binary_search_tree<tkey, tvalue>::node *&subtree_root,
        bool validate = true) const;
    
    void double_left_rotation(
        typename binary_search_tree<tkey, tvalue>::node *&subtree_root,
        bool at_grandparent_first,
        bool validate = true) const;
    
    void double_right_rotation(
        typename binary_search_tree<tkey, tvalue>::node *&subtree_root,
        bool at_grandparent_first,
        bool validate = true) const;
    
    // endregion subtree rotations definition
    
};

// region binary_search_tree<tkey, tvalue>::node methods implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::node::node(tkey const &key, tvalue const &value) : 
    key(key), value(value), left_subtree(nullptr), right_subtree(nullptr)
{}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::node::node(tkey const &key, tvalue &&value) : 
    key(key), value(std::move(value)), left_subtree(nullptr), right_subtree(nullptr)
{}

// endregion binary_search_tree<tkey, tvalue>::node methods implementation

// region iterators implementation

// region iterator data implementation


// endregion iterator data implementation

// region prefix_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::prefix_iterator::prefix_iterator(const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root):
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_prefix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::prefix_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::prefix_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_iterator &binary_search_tree<tkey, tvalue>::prefix_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Prefix iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

// постфиксный++
template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_iterator const binary_search_tree<tkey, tvalue>::prefix_iterator::operator++(int not_used)
{
    typename binary_search_tree<tkey, tvalue>::prefix_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data *binary_search_tree<tkey, tvalue>::prefix_iterator::operator*() const
{
    return _ptr;
}

// endregion prefix_iterator implementation

// region prefix_const_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::prefix_const_iterator::prefix_const_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_prefix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_const_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::prefix_const_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_const_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::prefix_const_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_iterator &binary_search_tree<tkey, tvalue>::prefix_const_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Prefix const iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_iterator const binary_search_tree<tkey, tvalue>::prefix_const_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::prefix_const_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data const *binary_search_tree<tkey, tvalue>::prefix_const_iterator::operator*() const
{
    return _ptr;
}

// endregion prefix_const_iterator implementation

// region prefix_reverse_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::prefix_reverse_iterator::prefix_reverse_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_prefix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_reverse_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::prefix_reverse_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_reverse_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::prefix_reverse_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_reverse_iterator &binary_search_tree<tkey, tvalue>::prefix_reverse_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Prefix reverse iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_reverse_iterator const binary_search_tree<tkey, tvalue>::prefix_reverse_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::prefix_reverse_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data *binary_search_tree<tkey, tvalue>::prefix_reverse_iterator::operator*() const
{
    return _ptr;
}

// endregion prefix_reverse_iterator implementation

// region prefix_const_reverse_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator::prefix_const_reverse_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_prefix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator &binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Prefix const reverse iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator const binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data const *binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator::operator*() const
{
    return _ptr;
}

// endregion prefix_const_reverse_iterator implementation

// region infix_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::infix_iterator::infix_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root):
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_infix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::infix_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::infix_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_iterator &binary_search_tree<tkey, tvalue>::infix_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Infix iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_iterator const binary_search_tree<tkey, tvalue>::infix_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::infix_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data *binary_search_tree<tkey, tvalue>::infix_iterator::operator*() const
{
    return _ptr;
}

// endregion infix_iterator implementation

// region infix_const_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::infix_const_iterator::infix_const_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    _tree->find_infix_path(0, _way, subtree_root);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_const_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::infix_const_iterator const &other) const noexcept
{
    return (_ptr -> depth == other._ptr -> depth) && (_ptr -> key == other._ptr -> key) && (_ptr -> value == other._ptr -> value);
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_const_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::infix_const_iterator const &other) const noexcept
{
    return !(*this == other);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_iterator &binary_search_tree<tkey, tvalue>::infix_const_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Infix const iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_iterator const binary_search_tree<tkey, tvalue>::infix_const_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::infix_const_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data const *binary_search_tree<tkey, tvalue>::infix_const_iterator::operator*() const
{
    return _ptr;
}

// endregion infix_const_iterator implementation

// region infix_reverse_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::infix_reverse_iterator::infix_reverse_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_infix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_reverse_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::infix_reverse_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_reverse_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::infix_reverse_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_reverse_iterator &binary_search_tree<tkey, tvalue>::infix_reverse_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Infix reverse iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_reverse_iterator const binary_search_tree<tkey, tvalue>::infix_reverse_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::infix_reverse_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data *binary_search_tree<tkey, tvalue>::infix_reverse_iterator::operator*() const
{
    return _ptr;
}

// endregion infix_reverse_iterator implementation

// region infix_const_reverse_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator::infix_const_reverse_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_infix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator &binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Infix const reverse iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator const binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data const *binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator::operator*() const
{
    return _ptr;
}

// endregion infix_const_reverse_iterator implementation

// region postfix_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::postfix_iterator::postfix_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_postfix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::postfix_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::postfix_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_iterator &binary_search_tree<tkey, tvalue>::postfix_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Postfix iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_iterator const binary_search_tree<tkey, tvalue>::postfix_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::postfix_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data *binary_search_tree<tkey, tvalue>::postfix_iterator::operator*() const
{
    return _ptr;
}

// endregion postfix_iterator implementation

// region postfix_const_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::postfix_const_iterator::postfix_const_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_postfix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_const_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::postfix_const_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_const_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::postfix_const_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_iterator &binary_search_tree<tkey, tvalue>::postfix_const_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Infix const iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_iterator const binary_search_tree<tkey, tvalue>::postfix_const_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::postfix_const_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data const *binary_search_tree<tkey, tvalue>::postfix_const_iterator::operator*() const
{
    return _ptr;
}

// endregion postfix_const_iterator implementation

// region postfix_reverse_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::postfix_reverse_iterator::postfix_reverse_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_postfix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_reverse_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::postfix_reverse_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_reverse_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::postfix_reverse_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_reverse_iterator &binary_search_tree<tkey, tvalue>::postfix_reverse_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Postfix reverse iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_reverse_iterator const binary_search_tree<tkey, tvalue>::postfix_reverse_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::postfix_reverse_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data *binary_search_tree<tkey, tvalue>::postfix_reverse_iterator::operator*() const
{
    return _ptr;
}

// endregion postfix_reverse_iterator implementation

// region postfix_const_reverse_iterator implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator::postfix_const_reverse_iterator(
    const binary_search_tree<tkey, tvalue> * tree, binary_search_tree<tkey, tvalue>::node *subtree_root) : 
    _tree(tree)
{
    if (subtree_root == nullptr)
    {
        _ptr = nullptr;
        return;
    }
    node * current = subtree_root;
    _tree->find_postfix_path(0, _way, current);
    _ptr = _way.front();
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator::operator==(
    typename binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator const &other) const noexcept
{
    return _ptr == other._ptr;
}

template<typename tkey, typename tvalue>
bool binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator::operator!=(
    typename binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator const &other) const noexcept
{
    return _ptr != other._ptr;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator &binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator::operator++()
{
    if (_way.empty()) _tree->error_with_guard("[BST] [Infix const reverse iterator] Way is empty");
    _tree->delete_iterator_data(_ptr);
    _way.pop();
    _ptr = _way.front();
    return *this;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator const binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator::operator++(
    int not_used)
{
    typename binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator tmp(*this);
    ++tmp;
    return tmp;
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::iterator_data const *binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator::operator*() const
{
    return _ptr;
}

// endregion postfix_const_reverse_iterator implementation

// endregion iterators implementation

// region target operations associated exception types implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_exception::insertion_of_existent_key_attempt_exception(
    tkey const &key):
    std::logic_error("Attempt to insert already existing key inside the tree.\n"), _key(key)
{}

template<typename tkey, typename tvalue>
tkey const &binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_exception::get_key() const noexcept
{
    return _key;
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::obtaining_of_nonexistent_key_attempt_exception::obtaining_of_nonexistent_key_attempt_exception(
    tkey const &key):
    std::logic_error("Attempt to obtain a value by non-existing key from the tree.\n"), _key(key)
{}

template<typename tkey, typename tvalue>
tkey const &binary_search_tree<tkey, tvalue>::obtaining_of_nonexistent_key_attempt_exception::get_key() const noexcept
{
    return _key;
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_exception::disposal_of_nonexistent_key_attempt_exception(
    tkey const &key):
    std::logic_error("Attempt to dispose a value by non-existing key from the tree.\n"), _key(key)
{}

template<typename tkey, typename tvalue>
tkey const &binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_exception::get_key() const noexcept
{
    return _key;
}

// endregion target operations associated exception types implementation

// region template methods implementation

// region binary_search_tree<tkey, tvalue>::template_method_basics implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::template_method_basics::template_method_basics(
    binary_search_tree<tkey, tvalue> *tree) : _tree(tree)
{}

template<typename tkey, typename tvalue>
[[nodiscard]] inline logger *binary_search_tree<tkey, tvalue>::template_method_basics::get_logger() const noexcept
{
    return _tree->get_logger();
}

// endregion binary_search_tree<tkey, tvalue>::template_method_basics implementation

// region search_tree<tkey, tvalue>::insertion_template_method implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::insertion_template_method::insertion_template_method(
    binary_search_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy):
    binary_search_tree<tkey, tvalue>::template_method_basics::template_method_basics(tree),
    _insertion_strategy(insertion_strategy),
    _tree(tree)
{}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::insertion_template_method::insert(
    tkey const &key,
    tvalue const &value)
{
    this->trace_with_guard("[BST] Trying to insert new node...\n");
    
    auto path = this -> find_path(key);
    if (*(path.top()))
    {
        switch (_insertion_strategy)
        {
            case insertion_of_existent_key_attempt_strategy::throw_an_exception:
                throw insertion_of_existent_key_attempt_exception(key);
            case insertion_of_existent_key_attempt_strategy::update_value:
                (*(path.top()))->value = value;
                break;
        }

        return;
    }

    node * raw_space = nullptr;

    try
    {
        raw_space = reinterpret_cast<node *>(this->allocate_with_guard(_tree->get_node_size(), 1));
        _tree->call_node_constructor(raw_space, key, value);
    }
    catch(const std::exception& e)
    {
        this->error_with_guard("[BST] BadAlloc corrupted while inserting node\n");
        throw e;
    }
    (*path.top()) = raw_space;
    this->balance(path);
    this->trace_with_guard("[BST] Node was inserted\n");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::insertion_template_method::insert(
    tkey const &key,
    tvalue &&value)
{
    this->trace_with_guard("[BST] Trying to insert new node...\n");
    auto path = this -> find_path(key);
    if (*(path.top()))
    {
        switch (_insertion_strategy)
        {
            case insertion_of_existent_key_attempt_strategy::throw_an_exception:
                throw insertion_of_existent_key_attempt_exception(key);
            case insertion_of_existent_key_attempt_strategy::update_value:
                (*(path.top()))->value = value;
                break;
        }

        return;
    }

    node * raw_space = nullptr;
    try
    {
        raw_space = reinterpret_cast<node *>(this->allocate_with_guard(_tree->get_node_size(), 1));
        _tree->call_node_constructor(raw_space, key, std::move(value));
    }
    catch(const std::exception& e)
    {
        this->error_with_guard("[BST] BadAlloc corrupted while inserting node\n");
        throw e;
    }
    (*path.top()) = raw_space;
    this->balance(path);
    this->trace_with_guard("[BST] Node was inserted\n");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::insertion_template_method::set_insertion_strategy(
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy) noexcept
{
    _insertion_strategy = insertion_strategy;
    this->trace_with_guard("[BST] New insertion strategy was setted\n");
}

template<typename tkey, typename tvalue>
allocator *binary_search_tree<tkey, tvalue>::insertion_template_method::get_allocator() const noexcept
{
    return this->_tree->get_allocator();
}

// endregion search_tree<tkey, tvalue>::insertion_template_method implementation

// region search_tree<tkey, tvalue>::obtaining_template_method implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::obtaining_template_method::obtaining_template_method(
    binary_search_tree<tkey, tvalue> *tree):
    binary_search_tree<tkey, tvalue>::template_method_basics::template_method_basics(tree), _tree(tree)
{}

template<typename tkey, typename tvalue>
tvalue const &binary_search_tree<tkey, tvalue>::obtaining_template_method::obtain(
    tkey const &key)
{
    auto path = this->find_path(key);
    if (*(path.top()) == nullptr)
    {
        throw obtaining_of_nonexistent_key_attempt_exception(key);
    }

    tvalue const &got_value = (*(path.top()))->value;

    return got_value;
}

// endregion search_tree<tkey, tvalue>::obtaining_template_method implementation

// region search_tree<tkey, tvalue>::disposal_template_method implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::disposal_template_method::disposal_template_method(
    binary_search_tree<tkey, tvalue> *tree,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
    binary_search_tree<tkey, tvalue>::template_method_basics::template_method_basics(tree), _tree(tree), _disposal_strategy(disposal_strategy)
{}

template<typename tkey, typename tvalue>
tvalue binary_search_tree<tkey, tvalue>::disposal_template_method::dispose(
    tkey const &key)
{
    this->trace_with_guard("[BST] Trying to remove node...");
    auto path = this->find_path(key);
    if (*(path.top()) == nullptr)
    {
        switch (_disposal_strategy)
        {
            case disposal_of_nonexistent_key_attempt_strategy::throw_an_exception:
                throw disposal_of_nonexistent_key_attempt_exception(key);
            case disposal_of_nonexistent_key_attempt_strategy::do_nothing:
                return tvalue();
        }
    }

    bool flag = false;

    if ((*(path.top()))->left_subtree != nullptr && (*(path.top()))->right_subtree != nullptr)
    {
        flag = true;
        this->trace_with_guard("[BST] Removing node has left and right subtrees\n");

        // searching for max element in left subtree
        auto *target_to_swap = *(path.top());
        auto **current = &((*(path.top()))->left_subtree);
        
        while (*current != nullptr)
        {
            path.push(current);
            current = &((*current)->right_subtree);
        }

        swap(std::move(target_to_swap->key), std::move((*(path.top()))->key));
        swap(std::move(target_to_swap->value), std::move((*(path.top()))->value));
    }

    tvalue value = std::move((*(path.top()))->value);
    node *leftover_subtree;

    if ((*(path.top()))->left_subtree == nullptr && (*(path.top()))->right_subtree == nullptr)
    {
        if (!flag) this->trace_with_guard("[BST] Removing node no subtrees\n");
        leftover_subtree = nullptr;
    }
    else if ((*(path.top()))->left_subtree != nullptr)
    {
        this->trace_with_guard("[BST] Removing node has left subtree\n");
        leftover_subtree = (*(path.top()))->left_subtree;
    }
    else
    {
        this->trace_with_guard("[BST] Removing node has right subtree\n");
        leftover_subtree = (*(path.top()))->right_subtree;
    }

    allocator::destruct(*(path.top()));
    deallocate_with_guard(*(path.top()));

    *(path.top()) = leftover_subtree;

    this->balance(path);

    this->trace_with_guard("[BST] Node was successfully removed\n");
    return value;
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::disposal_template_method::set_disposal_strategy(
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) noexcept
{
    _disposal_strategy = disposal_strategy;
    _tree->trace_with_guard("[BST] New disposal_strategy was setted\n");
}

template<typename tkey, typename tvalue>
[[nodiscard]] inline allocator *binary_search_tree<tkey, tvalue>::disposal_template_method::get_allocator() const noexcept
{
    return this->_tree->get_allocator();
}

// endregion search_tree<tkey, tvalue>::disposal_template_method implementation

// endregion template methods

// region construction, assignment, destruction implementation

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::binary_search_tree(
    typename binary_search_tree<tkey, tvalue>::insertion_template_method *insertion_template,
    typename binary_search_tree<tkey, tvalue>::obtaining_template_method *obtaining_template,
    typename binary_search_tree<tkey, tvalue>::disposal_template_method *disposal_template,
    std::function<int(tkey const &, tkey const &)> comparer,
    allocator *allocator,
    logger *logger):
    search_tree<tkey, tvalue>(comparer, logger, allocator),
    _insertion_template(insertion_template),
    _obtaining_template(obtaining_template),
    _disposal_template(disposal_template),
    _root(nullptr)
{
    this->debug_with_guard("[BST] Main constructor\n");
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::binary_search_tree(
    std::function<int(tkey const &, tkey const &)> keys_comparer,
    allocator *allocator,
    logger *logger,
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy,
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy):
    binary_search_tree(
        new binary_search_tree<tkey, tvalue>::insertion_template_method(this, insertion_strategy),
        new binary_search_tree<tkey, tvalue>::obtaining_template_method(this),
        new binary_search_tree<tkey, tvalue>::disposal_template_method(this, disposal_strategy),
        keys_comparer,
        allocator,
        logger)
{
    this->debug_with_guard("[BST] Constructor\n");
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::binary_search_tree(
    binary_search_tree<tkey, tvalue> const &other)
{
    this->debug_with_guard("[BST] [START] Copy constructor\n");

    if (_root != nullptr) clear(_root);
    _root = copy(other._root);

    if (_insertion_template != nullptr) delete _insertion_template;
    _insertion_template = new binary_search_tree<tkey, tvalue>::insertion_template_method(*other._insertion_template);
    if (_disposal_template != nullptr) delete _disposal_template;
    _disposal_template = new binary_search_tree<tkey, tvalue>::disposal_template_method(*other._disposal_template);
    if (_obtaining_template != nullptr) delete _obtaining_template;
    _obtaining_template = new binary_search_tree<tkey, tvalue>::obtaining_template_method(*other._obtaining_template);

    this->debug_with_guard("[BST] [END] Copy constructor\n");
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::binary_search_tree(
    binary_search_tree<tkey, tvalue> &&other) noexcept
{
    this->debug_with_guard("[BST] [START] Move constructor\n");
    if (_root != nullptr) clear(_root);
    _root = std::exchange(other._root, nullptr);

    if (_insertion_template != nullptr) delete _insertion_template;
    _insertion_template = std::exchange(other._insertion_template, nullptr);
    if (_disposal_template != nullptr) delete _disposal_template;
    _disposal_template = std::exchange(other._disposal_template, nullptr);
    if (_obtaining_template != nullptr) delete _obtaining_template;
    _obtaining_template = std::exchange(other._obtaining_template, nullptr);

    this->debug_with_guard("[BST] [END] Move constructor\n");
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue> &binary_search_tree<tkey, tvalue>::operator=(
    binary_search_tree<tkey, tvalue> const &other)
{
    this->debug_with_guard("[BST] [START] Assignment operator\n");
    if (this == &other)
    {
        this->debug_with_guard("[BST] [END] Assignment operator\n");
        return *this;
    }
    if (_root != nullptr) clear(_root);
    _root = copy(other._root);

    if (_insertion_template != nullptr) delete _insertion_template;
    _insertion_template = new binary_search_tree<tkey, tvalue>::insertion_template_method(*other._insertion_template);
    if (_disposal_template != nullptr) delete _disposal_template;
    _disposal_template = new binary_search_tree<tkey, tvalue>::disposal_template_method(*other._disposal_template);
    if (_obtaining_template != nullptr) delete _obtaining_template;
    _obtaining_template = new binary_search_tree<tkey, tvalue>::obtaining_template_method(*other._obtaining_template);

    this->debug_with_guard("[BST] [END] Assignment operator\n");
    return *this;
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue> &binary_search_tree<tkey, tvalue>::operator=(
    binary_search_tree<tkey, tvalue> &&other) noexcept
{
    this->debug_with_guard("[BST] [START] Move operator\n");
    if (this == &other)
    {
        this->debug_with_guard("[BST] [END] Move operator\n");
        return *this;
    }
    if (_root != nullptr) clear(_root);
    _root = std::exchange(other._root, nullptr);

    if (_insertion_template != nullptr) delete _insertion_template;
    _insertion_template = std::exchange(other._insertion_template, nullptr);
    if (_disposal_template != nullptr) delete _disposal_template;
    _disposal_template = std::exchange(other._disposal_template, nullptr);
    if (_obtaining_template != nullptr) delete _obtaining_template;
    _obtaining_template = std::exchange(other._obtaining_template, nullptr);
    this->debug_with_guard("[BST] [END] Move operator\n");
    return *this;
}

template<typename tkey, typename tvalue>
binary_search_tree<tkey, tvalue>::~binary_search_tree()
{
    this->debug_with_guard("[BST] [START] Destructor\n");
    clear(_root);
    if (_insertion_template != nullptr) delete _insertion_template;
    if (_obtaining_template != nullptr) delete _obtaining_template;
    if (_disposal_template != nullptr) delete _disposal_template;
    this->debug_with_guard("[BST] [END] Destructor\n");
}

// endregion construction, assignment, destruction implementation

// region associative_container<tkey, tvalue> contract implementation

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::insert(
    tkey const &key,
    tvalue const &value)
{
    this->debug_with_guard("[START] Insert\n");
    _insertion_template->insert(key, value);
    this->debug_with_guard("[END] Insert\n");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::insert(
    tkey const &key,
    tvalue &&value)
{
    this->debug_with_guard("[START] Insert with moving\n");
    _insertion_template->insert(key, std::move(value));
    this->debug_with_guard("[END] Insert with moving\n");
}

template<typename tkey, typename tvalue>
tvalue const &binary_search_tree<tkey, tvalue>::obtain(
    tkey const &key)
{
    this->debug_with_guard("[START] Obtain\n");
    return _obtaining_template->obtain(key);
    this->debug_with_guard("[END] Obtain\n");
}

template<typename tkey, typename tvalue>
std::vector<typename associative_container<tkey, tvalue>::key_value_pair> binary_search_tree<tkey, tvalue>::obtain_between(
    tkey const &lower_bound,
    tkey const &upper_bound,
    bool lower_bound_inclusive,
    bool upper_bound_inclusive)
{
    this->debug_with_guard("[BST] Obtain between\n");
    return _obtaining_template->obtain_between(lower_bound, upper_bound, lower_bound_inclusive, upper_bound_inclusive);
}

template<typename tkey, typename tvalue>
tvalue binary_search_tree<tkey, tvalue>::dispose(
    tkey const &key)
{
    this->debug_with_guard("Dispose\n");
    return _disposal_template->dispose(key);
}

// endregion associative_containers contract implementations

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::set_insertion_strategy(
    typename binary_search_tree<tkey, tvalue>::insertion_of_existent_key_attempt_strategy insertion_strategy) noexcept
{
    this->trace_with_guard("[START] Set insertion strategy\n");
    _insertion_template->set_insertion_strategy(insertion_strategy);
    this->trace_with_guard("[END] Set insertion strategy\n");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::set_removal_strategy(
    typename binary_search_tree<tkey, tvalue>::disposal_of_nonexistent_key_attempt_strategy disposal_strategy) noexcept
{
    this->trace_with_guard("[START] Set removal strategy\n");
    _disposal_template->set_disposal_strategy(disposal_strategy);
    this->trace_with_guard("[END] Set removal strategy\n");
}

// region iterators requesting implementation

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_iterator binary_search_tree<tkey, tvalue>::begin_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_iterator binary_search_tree<tkey, tvalue>::end_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_iterator binary_search_tree<tkey, tvalue>::cbegin_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_const_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_iterator binary_search_tree<tkey, tvalue>::cend_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_const_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_reverse_iterator binary_search_tree<tkey, tvalue>::rbegin_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_reverse_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_reverse_iterator binary_search_tree<tkey, tvalue>::rend_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_reverse_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator binary_search_tree<tkey, tvalue>::crbegin_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator binary_search_tree<tkey, tvalue>::crend_prefix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::prefix_const_reverse_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_iterator binary_search_tree<tkey, tvalue>::begin_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_iterator binary_search_tree<tkey, tvalue>::end_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_iterator binary_search_tree<tkey, tvalue>::cbegin_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_const_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_iterator binary_search_tree<tkey, tvalue>::cend_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_const_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_reverse_iterator binary_search_tree<tkey, tvalue>::rbegin_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_reverse_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_reverse_iterator binary_search_tree<tkey, tvalue>::rend_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_reverse_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator binary_search_tree<tkey, tvalue>::crbegin_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator binary_search_tree<tkey, tvalue>::crend_infix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::infix_const_reverse_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_iterator binary_search_tree<tkey, tvalue>::begin_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_iterator binary_search_tree<tkey, tvalue>::end_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_iterator binary_search_tree<tkey, tvalue>::cbegin_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_const_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_iterator binary_search_tree<tkey, tvalue>::cend_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_const_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_reverse_iterator binary_search_tree<tkey, tvalue>::rbegin_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_reverse_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_reverse_iterator binary_search_tree<tkey, tvalue>::rend_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_reverse_iterator(this, nullptr);
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator binary_search_tree<tkey, tvalue>::crbegin_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator(this, dynamic_cast<typename binary_search_tree<tkey, tvalue>::node *>(_root));
}

template<typename tkey, typename tvalue>
typename binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator binary_search_tree<tkey, tvalue>::crend_postfix() const noexcept
{
    return binary_search_tree<tkey, tvalue>::postfix_const_reverse_iterator(this, nullptr);
}


// endregion iterators request implementation

// region subtree rotations implementation

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::small_left_rotation(
    binary_search_tree<tkey, tvalue>::node *&subtree_root,
    bool validate) const
{
    this->debug_with_guard("[START] Small Left Rotation");
    if (subtree_root == nullptr)
    {
        this->debug_with_guard("[END] Small Left Rotation");
        return;
    }
    if (validate && subtree_root->right_subtree == nullptr)
    {
        std::string error = "Can't perform rotation\n";
        this->error_with_guard(error);
        throw std::logic_error(error);
    }

    node * new_root = subtree_root->right_subtree;
    subtree_root->right_subtree = new_root->left_subtree;
    new_root->left_subtree = subtree_root;
    subtree_root = new_root;

    this->debug_with_guard("[END] Small Left Rotation");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::small_right_rotation(
    binary_search_tree<tkey, tvalue>::node *&subtree_root,
    bool validate) const
{
    this->debug_with_guard("[START] Small Right Rotation");
    if (subtree_root == nullptr)
    {
        this->debug_with_guard("[END] Small Right Rotation");
        return;
    }
    if (validate && subtree_root->left_subtree == nullptr)
    {
        std::string error = "Can't perform rotation\n";
        this->error_with_guard(error);
        throw std::logic_error(error);
    }
    node * new_root = subtree_root->left_subtree;
    subtree_root->left_subtree = new_root->right_subtree;
    new_root->right_subtree = subtree_root;
    subtree_root = new_root;

    this->debug_with_guard("[END] Small Right Rotation");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::big_left_rotation(
    binary_search_tree<tkey, tvalue>::node *&subtree_root,
    bool validate) const
{
    this->debug_with_guard("[START] Big Left Rotation");
    if (subtree_root == nullptr)
    {
        this->debug_with_guard("[END] Big Left Rotation");
        return;
    }

    try
    {
        small_right_rotation(subtree_root->right_subtree, validate);
        small_left_rotation(subtree_root, validate);
    }
    catch(const std::logic_error & e)
    {
        throw e;
    }
    
    this->debug_with_guard("[END] Big Left Rotation");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::big_right_rotation(
    binary_search_tree<tkey, tvalue>::node *&subtree_root,
    bool validate) const
{
    this->debug_with_guard("[START] Big Right Rotation");
    if (subtree_root == nullptr)
    {
        this->debug_with_guard("[END] Big Right Rotation");
        return;
    }

    try
    {
        small_left_rotation(subtree_root->left_subtree, validate);
        small_right_rotation(subtree_root, validate);
    }
    catch(const std::logic_error& e)
    {
        throw e;
    }

    this->debug_with_guard("[END] Big Right Rotation");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::double_left_rotation(
    binary_search_tree<tkey, tvalue>::node *&subtree_root,
    bool at_grandparent_first,
    bool validate) const
{
    this->debug_with_guard("[START] Double Left Rotation");
    try
    {
        if (at_grandparent_first) small_left_rotation(subtree_root, validate);
        else small_left_rotation(subtree_root -> right_subtree, validate);
        small_left_rotation(subtree_root, validate);
    }
    catch(const std::logic_error& e)
    {
        throw e;
    }

    this->debug_with_guard("[END] Double Left Rotation");
}

template<typename tkey, typename tvalue>
void binary_search_tree<tkey, tvalue>::double_right_rotation(
    binary_search_tree<tkey, tvalue>::node *&subtree_root,
    bool at_grandparent_first,
    bool validate) const
{
    this->debug_with_guard("[START] Double Right Rotation");
    
    try
    {
        if (at_grandparent_first) small_right_rotation(subtree_root, validate);
        else small_right_rotation(subtree_root -> right_subtree, validate);
        small_right_rotation(subtree_root, validate);
    }
    catch(const std::logic_error& e)
    {
        throw e;
    }

    this->debug_with_guard("[END] Double Right Rotation");
}

// endregion subtree rotations implementation

#endif //MATH_PRACTICE_AND_OPERATING_SYSTEMS_BINARY_SEARCH_TREE_H
