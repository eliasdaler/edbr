* Replace VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL with VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
* Replace VK_IMAGE_LAYOUT_COLOR/DEPTH_ATTACHMENT_OPTIMAL with VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL

* Audio: it would be better to send audio events instead of calling audioManager->playSound directly - the dependency of many classes on the AudioManager would be removed
