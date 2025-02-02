#include "layer.hpp"

#include <algorithm>

Layer::Layer(unsigned int id) : id_{id} {
}

unsigned int Layer::ID() const {
	return id_;
}

Layer& Layer::SetWindow(const std::shared_ptr<Window>& window) {
	window_ = window;
	return *this;
}

std::shared_ptr<Window> Layer::GetWindow() const {
	return window_;
}

Layer& Layer::Move(Vector2D<int> pos) {
	pos_ = pos;
	return *this;
}

Layer& Layer::MoveRelative(Vector2D<int> pos_diff) {
	pos_ += pos_diff;
	return *this;
}

void Layer::DrawTo(FrameBuffer& screen) const {
	// shared_ptr type can implicit conversion to bool
	if (window_) {
		window_->DrawTo(screen, pos_);
	}
}

void LayerManager::SetScreen(FrameBuffer* screen) {
	screen_ = screen;
}

Layer& LayerManager::NewLayer() {
	++latest_id_;
	// emplace_back represent that add parameter to array and return pointer of added content.
	// but type of this content is std::unique_ptr so it can't share. so add * and, change it to reference type.
	// add * to std::unique_ptr, we can get reference type.
	return *layers_.emplace_back(new Layer{latest_id_});
}

void LayerManager::Draw() const {
	for (auto layer : layer_stack_) {
		layer->DrawTo(*screen_);
	}
}

void LayerManager::Move(unsigned int id, Vector2D<int> new_position) {
	FindLayer(id)->Move(new_position);
}

void LayerManager::MoveRelative(unsigned int id, Vector2D<int> pos_diff) {
	FindLayer(id)->MoveRelative(pos_diff);
}

// change index of layer specified with first parameter to new index specified with second parameter.
void LayerManager::UpDown(unsigned int id, int new_height) {
	if (new_height < 0) {
		Hide(id);
		return;
	}

	auto layer = FindLayer(id);
	auto old_pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);
	// iterator + integer return iterator.
	auto new_pos = layer_stack_.begin() + new_height;

	if (old_pos == layer_stack_.end()) {
		layer_stack_.insert(new_pos, layer);
		return;
	}

	layer_stack_.erase(old_pos);
	new_pos = layer_stack_.begin() + std::min<size_t>(static_cast<size_t>(new_height), layer_stack_.size() - 1);
	// first prameter of insert() is not index, it's iterator. so if first parameter is 2, insert layer as second element.
	layer_stack_.insert(new_pos, layer);
}

void LayerManager::Hide(unsigned int id) {
	auto layer = FindLayer(id);
	auto pos = std::find(layer_stack_.begin(), layer_stack_.end(), layer);

	if (pos != layer_stack_.end()) {
		layer_stack_.erase(pos);
	}
}

Layer* LayerManager::FindLayer(unsigned int id) {
	// this is predicate. to use local variable, set loval variable to [].
	auto pred = [id](const std::unique_ptr<Layer>& elem) {
		return elem ->ID() == id;
	};

	// third parameter of std::find_if is predicate. 
	// parameter is each element between first parameter to second parameter.
	// find_if find first element that pred return true.
	// find_if return iterator.
	auto it = std::find_if(layers_.begin(), layers_.end(), pred);

	// end() represent next iterator of last iterator.
	// so result of end() and last iteratpr are not same iterator.
	if (it == layers_.end()) {
		return nullptr;
	}

	return it->get();
}

LayerManager* layer_manager;
