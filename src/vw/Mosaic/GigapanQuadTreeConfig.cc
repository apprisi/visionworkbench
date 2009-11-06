// __BEGIN_LICENSE__
// Copyright (C) 2006-2009 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


#include <vw/Mosaic/GigapanQuadTreeConfig.h>

#include <boost/bind.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>
namespace fs = boost::filesystem;

namespace vw {
namespace mosaic {

  struct GigapanQuadTreeConfigData {
    BBox2 m_longlat_bbox;


    std::vector<std::pair<std::string,vw::BBox2i> > branch_func( QuadTreeGenerator const&, std::string const& name, BBox2i const& region ) const;
    void metadata_func( QuadTreeGenerator const&, QuadTreeGenerator::TileInfo const& info ) const;

    public:
      GigapanQuadTreeConfigData() : m_longlat_bbox(0, 0, 0, 0) {}
  };

  GigapanQuadTreeConfig::GigapanQuadTreeConfig()
    : m_data( new GigapanQuadTreeConfigData() )
  {}

  std::string GigapanQuadTreeConfig::image_path( QuadTreeGenerator const& qtree, std::string const& name ) {
    return QuadTreeGenerator::tiered_image_path()(qtree, name);
  }

  void GigapanQuadTreeConfig::configure( QuadTreeGenerator& qtree ) const {
    qtree.set_image_path_func( &image_path );
    qtree.set_cull_images( true );
    qtree.set_metadata_func( boost::bind(&GigapanQuadTreeConfigData::metadata_func,m_data,_1,_2) );
    
    if (m_data->m_longlat_bbox.width() != 0 || m_data->m_longlat_bbox.height() != 0) {
      qtree.set_branch_func( boost::bind(&GigapanQuadTreeConfigData::branch_func,m_data,_1,_2,_3) );
    }
  }

  void GigapanQuadTreeConfig::set_longlat_bbox( BBox2 const& bbox ) {
    m_data->m_longlat_bbox = bbox;
  }

  std::vector<std::pair<std::string,vw::BBox2i> > GigapanQuadTreeConfigData::branch_func( QuadTreeGenerator const& qtree, std::string const& name, BBox2i const& region ) const {
    std::vector<std::pair<std::string,vw::BBox2i> > children;
    if( region.height() > qtree.get_tile_size() ) {

      Vector2i dims = qtree.get_dimensions();
      double aspect_ratio = 2 * (region.width()/region.height()) * ( (m_longlat_bbox.width()/dims.x()) / (m_longlat_bbox.height()/dims.y()) );

      double bottom_lat = m_longlat_bbox.max().y() - region.max().y()*m_longlat_bbox.height() / dims.y();
      double top_lat = m_longlat_bbox.max().y() - region.min().y()*m_longlat_bbox.height() / dims.y();
      bool top_merge = ( bottom_lat > 0 ) && ( ( 1.0 / cos(M_PI/180 * bottom_lat) ) > aspect_ratio );
      bool bottom_merge = ( top_lat < 0 ) && ( ( 1.0 / cos(M_PI/180 * top_lat) ) > aspect_ratio );

      if( top_merge ) {
        children.push_back( std::make_pair( name + "4", BBox2i( region.min(), region.max() - Vector2i(0,region.height()/2) ) ) );
      }
      else {
        children.push_back( std::make_pair( name + "0", BBox2i( (region + region.min()) / 2 ) ) );
        children.push_back( std::make_pair( name + "1", BBox2i( (region + Vector2i(region.max().x(),region.min().y())) / 2 ) ) );
      }
      if( bottom_merge ) {
        children.push_back( std::make_pair( name + "5", BBox2i( region.min() + Vector2i(0,region.height()/2), region.max() ) ) );
      }
      else {
        children.push_back( std::make_pair( name + "2", BBox2i( (region + Vector2i(region.min().x(),region.max().y())) / 2 ) ) );
        children.push_back( std::make_pair( name + "3", BBox2i( (region + region.max()) / 2 ) ) );
      }
    }
    return children;
  }

  void GigapanQuadTreeConfigData::metadata_func( QuadTreeGenerator const& qtree, QuadTreeGenerator::TileInfo const& info ) const {
    bool root_node = ( info.name.size() == 0 );
    
    
    if ( root_node) { 
      std::ostringstream json;
      fs::path file_path( info.filepath, fs::native );
      int base_len = file_path.branch_path().native_file_string().size() + 1;
      fs::path json_path = change_extension( file_path, ".json" );

      // TODO: Make this an actual json file
      json << "<begin>" << std::endl;
      json << "Num levels: " << qtree.get_tree_levels() << std::endl;
      json << "Dimensions: " << qtree.get_dimensions() << std::endl;
      json << "<end>" << std::endl;    
      fs::ofstream jsonfs(json_path);
      jsonfs << json.str();  
    }
  }
} // namespace mosaic
} // namespace vw
